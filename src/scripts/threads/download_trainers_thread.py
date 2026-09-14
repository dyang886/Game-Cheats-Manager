import ctypes
import filecmp
import os
import re
import shutil
import subprocess
import time
from ctypes import wintypes

from PyQt6.QtCore import pyqtSignal

from config import *
from threads.download_base_thread import DownloadBaseThread


class DownloadTrainersThread(DownloadBaseThread):
    ceInstalled = pyqtSignal(str)
    installed = pyqtSignal(list)  # names of the trainer folders this download created

    def __init__(self, index, trainers, trainerDownloadPath, update_entry, parent=None):
        super().__init__(parent)
        self.index = index
        self.trainers = trainers
        self.trainerDownloadPath = trainerDownloadPath
        self.update_entry = update_entry
        self.download_finish_delay = 0.5
        self.update_error_delay = 3
        self.bgMusicMessageSent = False

    def run(self):
        try:
            try:
                if os.path.exists(DOWNLOAD_TEMP_DIR):
                    shutil.rmtree(DOWNLOAD_TEMP_DIR)
                os.makedirs(DOWNLOAD_TEMP_DIR, exist_ok=True)
            except Exception as error:
                print(f"Could not prepare temporary download folder: {error}")
                self.message.emit(tr("Could not prepare the temporary download folder. Check its permissions and your antivirus software."), "failure")
                time.sleep(self.update_error_delay)
                self.finished.emit(1)
                return

            self.src_dst = []  # List content: { "src": source_path, "dst": destination_path, "version": YYYY.MM.DD }
            self.expected_source_files = {}
            self.instructionDst = ""
            selected_trainer = None
            if not self.update_entry:
                selected_trainer = DownloadBaseThread.trainer_urls[self.index]
            else:
                selected_trainer = self.update_entry
            origin = selected_trainer["origin"]

            result = True
            if origin == "fling_main" or origin == "fling_archive":
                result = self.download_fling(selected_trainer)
            elif origin == "xiaoxing":
                result = self.download_xiaoxing(selected_trainer)
            elif origin in ["the_cheat_script", "ct_other", "gcm", "other"]:
                result = self.download_default(selected_trainer)

            if not result:
                return

            try:
                self.install_prepared_trainer(selected_trainer)
            except Exception as error:
                self.message.emit(tr("An error occurred when installing trainer: ") + str(error), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return

            if self.instructionDst and not self.update_entry:
                self.messageBox.emit("info", tr("Attention"), tr("This trainer requires additional setup before use. Please check the opened folder for instructions.\nThe instructions are always stored in the 'gcm-instructions' folder."))
                try:
                    os.startfile(self.instructionDst)
                except OSError as error:
                    print(f"Could not open trainer instructions: {error}")

            if self.is_cheat_engine_package(selected_trainer):
                self.report_cheat_engine_install()
            self.installed.emit(self.installed_trainer_names())
            self.message.emit(tr("Download success!"), "success")
            time.sleep(self.download_finish_delay)
            self.finished.emit(0)

        except Exception as error:
            print(f"Could not prepare trainer: {error}")
            self.message.emit(tr("Could not prepare the downloaded trainer. Please try again and check your antivirus software."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)

    @staticmethod
    def file_tree(root):
        """Return the relative path and size of every file below root."""
        root = os.path.abspath(root)
        if not os.path.isdir(root):
            raise FileNotFoundError(root)

        tree = {}
        for current_root, _, files in os.walk(root):
            for filename in files:
                path = os.path.join(current_root, filename)
                relative_path = os.path.normpath(os.path.relpath(path, root))
                tree[relative_path] = os.path.getsize(path)
        return tree

    @staticmethod
    def extract_archive(archive_path, extraction_root, ignored_paths=()):
        """Read the archive file tree, then extract it."""
        archive_error = tr("The downloaded archive is invalid or incomplete. Your antivirus software may have blocked or removed it.")
        try:
            result = subprocess.run(
                [unzip_path, "l", "-slt", "-ba", "-sccUTF-8", archive_path],
                check=True,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                creationflags=subprocess.CREATE_NO_WINDOW
            )
        except Exception as error:
            print(f"Could not read archive file tree: {error}")
            raise RuntimeError(archive_error) from error

        ignored = {os.path.normcase(os.path.normpath(path)) for path in ignored_paths}
        extraction_root = os.path.abspath(extraction_root)
        tree = {}
        record = {}

        def add_record():
            path = record.get("Path")
            if not path:
                return

            relative_path = os.path.normpath(path.replace("/", os.sep))
            if os.path.normcase(relative_path) in ignored:
                return
            if record.get("Folder") == "+" or "Size" not in record:
                return
            destination = os.path.abspath(os.path.join(extraction_root, relative_path))
            tree[destination] = int(record["Size"])

        for line in result.stdout.splitlines() + [""]:
            if not line:
                add_record()
                record = {}
            elif " = " in line:
                key, value = line.split(" = ", 1)
                record[key] = value

        if not tree:
            print("Archive file tree is empty.")
            raise RuntimeError(archive_error)

        try:
            subprocess.run(
                [unzip_path, "x", "-y", archive_path, f"-o{extraction_root}"],
                check=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
        except Exception as error:
            print(f"Could not extract archive: {error}")
            raise RuntimeError(archive_error) from error
        return tree

    @staticmethod
    def validate_files(expected):
        if not expected:
            raise RuntimeError("No trainer files were available for verification.")
        for path, size in expected.items():
            try:
                actual_size = os.path.getsize(path) if os.path.isfile(path) else None
            except OSError as error:
                raise RuntimeError(f"Could not inspect trainer file '{path}': {error}") from error
            if actual_size != size:
                raise RuntimeError(f"Trainer file '{path}' has {actual_size} bytes; expected {size}.")

    def restore_backup(self, backup_directory, trainer_directory, expected_tree):
        """Mirror a backup into the trainer directory and verify the restored files."""
        if self.file_tree(backup_directory) != expected_tree:
            raise RuntimeError(f"Trainer backup '{backup_directory}' no longer matches its verified file tree.")
        filecmp.clear_cache()

        def remove_entry(path):
            if os.path.isdir(path) and not os.path.islink(path):
                shutil.rmtree(path)
            elif os.path.lexists(path):
                os.remove(path)

        if (os.path.lexists(trainer_directory)
                and (not os.path.isdir(trainer_directory) or os.path.islink(trainer_directory))):
            remove_entry(trainer_directory)
        os.makedirs(trainer_directory, exist_ok=True)

        # Remove files that did not exist in the backup and conflicting directories.
        for current_root, directories, files in os.walk(trainer_directory, topdown=False):
            for filename in files:
                path = os.path.join(current_root, filename)
                if os.path.normpath(os.path.relpath(path, trainer_directory)) not in expected_tree:
                    os.remove(path)
            for directory in directories:
                path = os.path.join(current_root, directory)
                relative_path = os.path.normpath(os.path.relpath(path, trainer_directory))
                backup_path = os.path.join(backup_directory, relative_path)
                if os.path.islink(path) or os.path.isfile(backup_path):
                    remove_entry(path)
                elif not os.listdir(path):
                    os.rmdir(path)

        # Copy only missing or different files, leaving identical open files untouched.
        for relative_path, size in expected_tree.items():
            source = os.path.join(backup_directory, relative_path)
            destination = os.path.join(trainer_directory, relative_path)
            if not os.path.isfile(source) or os.path.getsize(source) != size:
                raise RuntimeError(f"Trainer backup file '{source}' failed verification.")
            if os.path.islink(destination) or (os.path.lexists(destination) and not os.path.isfile(destination)):
                remove_entry(destination)
            if os.path.isfile(destination) and filecmp.cmp(source, destination, shallow=False):
                continue
            os.makedirs(os.path.dirname(destination), exist_ok=True)
            shutil.copy2(source, destination)

        if self.file_tree(trainer_directory) != expected_tree:
            raise RuntimeError(f"Restored trainer '{trainer_directory}' does not match the backup file tree.")
        filecmp.clear_cache()
        for relative_path in expected_tree:
            if not filecmp.cmp(
                os.path.join(backup_directory, relative_path),
                os.path.join(trainer_directory, relative_path),
                shallow=False
            ):
                raise RuntimeError(f"Restored trainer file '{relative_path}' does not match its backup.")

    def install_prepared_trainer(self, selected_trainer):
        """Install verified content and restore a verified backup directly on failure."""
        trainer_directory = os.path.abspath(self.update_entry["trainer_dir"]) if self.update_entry else None
        download_root = os.path.abspath(self.trainerDownloadPath)
        backup_root = os.path.join(download_root, TRAINER_BACKUP_DIRECTORY)
        backup_paths = []
        backups = {}
        backups_ready = False
        retained_backups = set()
        placement_started = False
        cleanup_roots = []
        install_error_message = tr("Could not install the trainer files. Close any running trainer and check your antivirus software.")
        failure_message = install_error_message

        def remove_path(path):
            if os.path.isdir(path) and not os.path.islink(path):
                shutil.rmtree(path)
            elif os.path.lexists(path):
                os.remove(path)

        try:
            # Verify the prepared sources and map their tree through src_dst.
            self.validate_files(self.expected_source_files)
            mappings = sorted(
                ((os.path.abspath(item["src"]), os.path.abspath(item["dst"])) for item in self.src_dst),
                key=lambda pair: len(pair[0]),
                reverse=True
            )
            expected_destinations = {}
            for source, size in self.expected_source_files.items():
                for source_root, destination_root in mappings:
                    relative_path = os.path.relpath(source, source_root)
                    if relative_path == ".":
                        destination = destination_root
                    elif relative_path == os.pardir or relative_path.startswith(os.pardir + os.sep):
                        continue
                    else:
                        destination = os.path.abspath(os.path.join(destination_root, relative_path))
                    expected_destinations[destination] = size
                    break
                else:
                    raise RuntimeError(f"No installation mapping contains trainer file '{source}'.")

            # Remember the top-level destinations to verify and clean on failure.
            destination_roots = []
            for _, destination in mappings:
                relative_path = os.path.relpath(destination, download_root)
                root = os.path.join(download_root, relative_path.split(os.sep, 1)[0])
                if root not in destination_roots:
                    destination_roots.append(root)

            existing_directories = []
            if trainer_directory:
                existing_directories.append(trainer_directory)
            for root in destination_roots:
                if os.path.lexists(root) and not any(
                    os.path.normcase(root) == os.path.normcase(path) for path in existing_directories
                ):
                    existing_directories.append(root)

            cleanup_roots = [
                root for root in destination_roots
                if not any(os.path.normcase(root) == os.path.normcase(path) for path in existing_directories)
            ]

            if existing_directories:
                failure_message = tr("Could not create a complete trainer backup. The existing installation was not changed. Your antivirus software may have interfered.")
                os.makedirs(backup_root, exist_ok=True)
                for existing_directory in existing_directories:
                    backup_directory = os.path.join(backup_root, os.path.basename(existing_directory))
                    backup_paths.append(backup_directory)
                    original_tree = self.file_tree(existing_directory)
                    if os.path.lexists(backup_directory):
                        retained_backups.add(backup_directory)
                        print("Reusing existing trainer backup.")
                    else:
                        shutil.copytree(existing_directory, backup_directory)

                    backup_tree = self.file_tree(backup_directory)
                    filecmp.clear_cache()
                    if backup_tree != original_tree or any(
                        not filecmp.cmp(
                            os.path.join(existing_directory, path),
                            os.path.join(backup_directory, path),
                            shallow=False
                        )
                        for path in original_tree
                    ):
                        raise RuntimeError(f"Trainer backup '{backup_directory}' does not match the installed trainer.")
                    retained_backups.discard(backup_directory)
                    backups[existing_directory] = (backup_directory, original_tree)

                backups_ready = True
                failure_message = tr("Could not remove the existing trainer. Close it if it is running, check your antivirus software, and try again.")
                for existing_directory in existing_directories:
                    remove_path(existing_directory)
                failure_message = install_error_message

            info = {
                "game_name": selected_trainer["game_name"],
                "origin": selected_trainer["origin"]
            }
            if selected_trainer.get("version"):
                info["version"] = selected_trainer["version"]
            if selected_trainer["origin"] in ["other", "ct_other"]:
                info["gcm_url"] = selected_trainer["url"]
            if selected_trainer.get("extension"):
                info["extension"] = selected_trainer["extension"]

            placement_started = True
            info_directories = set()
            for item in self.src_dst:
                if os.path.isfile(item["src"]):
                    destination_directory = os.path.dirname(item["dst"])
                    os.makedirs(destination_directory, exist_ok=True)
                else:
                    destination_directory = item["dst"]
                shutil.move(item["src"], item["dst"])
                if os.path.normpath(destination_directory) != os.path.normpath(self.instructionDst):
                    info_directories.add(destination_directory)

            for destination_directory in info_directories:
                info_path = os.path.join(destination_directory, "gcm_info.json")
                with open(info_path, "w", encoding="utf-8") as info_file:
                    json.dump(info, info_file, ensure_ascii=False, indent=4)
                expected_destinations[os.path.abspath(info_path)] = os.path.getsize(info_path)

            self.validate_files(expected_destinations)
            for root in destination_roots:
                expected_tree = {}
                for destination, size in expected_destinations.items():
                    relative_path = os.path.normpath(os.path.relpath(destination, root))
                    if relative_path != os.pardir and not relative_path.startswith(os.pardir + os.sep):
                        expected_tree[relative_path] = size
                if self.file_tree(root) != expected_tree:
                    raise RuntimeError(f"Installed trainer '{root}' does not match its expected file tree.")

        except Exception as install_error:
            print(f"Trainer installation failed: {install_error}")
            cleanup_errors = []
            if placement_started:
                for path in cleanup_roots:
                    try:
                        remove_path(path)
                    except Exception as cleanup_error:
                        cleanup_errors.append(cleanup_error)

            rollback_errors = []
            if backups_ready:
                for existing_directory, (backup_directory, original_tree) in backups.items():
                    try:
                        self.restore_backup(backup_directory, existing_directory, original_tree)
                    except Exception as error:
                        rollback_errors.append(error)
                        if os.path.lexists(backup_directory):
                            retained_backups.add(backup_directory)
                backups_ready = False

            if cleanup_errors and backups and not rollback_errors:
                rollback_errors.append(RuntimeError(f"Could not remove incomplete trainer paths: {cleanup_errors}"))
                retained_backups.update(path for path in backup_paths if os.path.lexists(path))

            if rollback_errors:
                print(f"Could not restore trainer: {rollback_errors[0]}")
                raise RuntimeError(
                    tr("The previous trainer installation could not be fully restored. Check your antivirus quarantine and the '.gcm-backup' folder.")
                ) from install_error
            if cleanup_errors:
                print(f"Could not remove incomplete trainer files: {cleanup_errors[0]}")
            raise RuntimeError(failure_message) from install_error

        finally:
            for backup_directory in backup_paths:
                if backup_directory not in retained_backups and os.path.isdir(backup_directory):
                    try:
                        shutil.rmtree(backup_directory)
                    except OSError as error:
                        print(f"Could not clean up trainer backup: {error}")

            try:
                if os.path.isdir(backup_root) and not os.listdir(backup_root):
                    os.rmdir(backup_root)
            except OSError as error:
                print(f"Could not clean up trainer backup: {error}")

    def report_cheat_engine_install(self):
        for item in self.src_dst:
            destination = item["dst"]
            if os.path.isdir(destination) and os.path.isfile(os.path.join(destination, CE_EXECUTABLE)):
                self.ceInstalled.emit(os.path.normpath(destination))
                return

    @staticmethod
    def format_request_error(error):
        message = tr("Internet request failed.")
        pending = [error]
        visited = set()

        while pending:
            current = pending.pop(0)
            if not isinstance(current, BaseException) or id(current) in visited:
                continue
            visited.add(id(current))

            response = getattr(current, "response", None)
            status_code = getattr(response, "status_code", None)
            if status_code is not None:
                return f"{message} (HTTP {status_code})"

            winerror = getattr(current, "winerror", None)
            if winerror is not None:
                return f"{message} (WinError {winerror})"

            errno = getattr(current, "errno", None)
            if errno is not None:
                return f"{message} (errno {errno})"

            pending.extend(
                nested
                for nested in (
                    getattr(current, "reason", None),
                    current.__cause__,
                    current.__context__,
                    *current.args,
                )
                if isinstance(nested, BaseException)
            )

        return message

    def installed_trainer_names(self):
        names = []
        for item in self.src_dst:
            relative = os.path.relpath(item["dst"], self.trainerDownloadPath)
            name = os.path.splitext(relative.split(os.sep)[0])[0]
            if name not in names:
                names.append(name)
        return names

    def handle_multi_version_archive(self, extractedContentPath, trainerName_display, selected_trainer):
        # An extension points at one trainer file, so it can never describe a multi-version package
        if selected_trainer.get("extension", "").strip():
            return False

        # Instructions at the root belong to the package as a whole, not to a single version
        if os.path.isdir(os.path.join(extractedContentPath, "gcm-instructions")):
            return False

        # A multi-version package holds its version folders and nothing else
        temp_contents = sorted(os.listdir(extractedContentPath))
        if not temp_contents or not all(os.path.isdir(os.path.join(extractedContentPath, item)) for item in temp_contents):
            return False

        for folder_name in temp_contents:
            source_path = os.path.join(extractedContentPath, folder_name)
            # Add folder name as suffix
            safe_folder_name = self.symbol_replacement(folder_name.strip())
            destination_path = os.path.join(self.trainerDownloadPath, f"{trainerName_display} {safe_folder_name}")
            self.src_dst.append({"src": source_path, "dst": destination_path})

            # Each version may ship its own instructions; the first one found is the one opened
            if not self.instructionDst and os.path.isdir(os.path.join(source_path, "gcm-instructions")):
                self.instructionDst = os.path.join(destination_path, "gcm-instructions")

        return True

    def download_default(self, selected_trainer):
        if self.update_entry:
            trainerName_display = selected_trainer["trainer_name"]
            self.message.emit(tr("Updating ") + trainerName_display + "...", None)
        else:
            trainerName_display = self.symbol_replacement(selected_trainer["trainer_name"])
            # Trainer duplication check
            for trainerPath in self.trainers.keys():
                if self.symbol_replacement(selected_trainer["trainer_name"]) == os.path.splitext(os.path.basename(trainerPath))[0]:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return False

        self.message.emit(tr("Downloading..."), "download")
        extractedContentPath = os.path.join(DOWNLOAD_TEMP_DIR, "extracted")
        try:
            signed_url = self.get_signed_download_url(selected_trainer['url'], raise_errors=True)
            trainerTemp = self.request_download(signed_url, DOWNLOAD_TEMP_DIR, raise_errors=True)
            if not trainerTemp:
                raise Exception(tr("Internet request failed."))
            self.expected_source_files = {os.path.abspath(trainerTemp): os.path.getsize(trainerTemp)}

        except Exception as e:
            self.message.emit(self.format_request_error(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Extract compressed file if not single exe
        extracted = False
        if os.path.splitext(trainerTemp)[1].lower() in ARCHIVE_EXTENSIONS:
            extracted = True
            self.message.emit(tr("Decompressing..."), None)
            try:
                self.expected_source_files = self.extract_archive(trainerTemp, extractedContentPath)

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

            try:
                os.remove(trainerTemp)
            except OSError as error:
                print(f"Could not remove extracted archive: {error}")

        if extracted:
            # Set instruction destination if gcm-instructions folder present at root
            instructionsFolder = os.path.join(extractedContentPath, "gcm-instructions")
            if os.path.isdir(instructionsFolder):
                self.instructionDst = os.path.join(self.trainerDownloadPath, trainerName_display, "gcm-instructions")

            # If the archive contains multiple version folders, split them up into multiple dest folders
            if not self.handle_multi_version_archive(extractedContentPath, trainerName_display, selected_trainer):
                destination_path = os.path.join(self.trainerDownloadPath, trainerName_display)
                self.src_dst.append({"src": extractedContentPath, "dst": destination_path})
        else:
            destination_path = os.path.join(self.trainerDownloadPath, trainerName_display)
            self.src_dst.append({"src": trainerTemp, "dst": os.path.join(destination_path, os.path.basename(trainerTemp))})

        return True

    def modify_fling_settings(self, removeBgMusic):
        def modify_midi(settingsDir):
            bgMusicPath = os.path.join(settingsDir, "TrainerBGM.mid")
            if not os.path.isfile(bgMusicPath):
                return

            if removeBgMusic:
                shutil.copyfile(emptyMidi_path, bgMusicPath)
            else:
                os.remove(bgMusicPath)

        def modify_ini_files(settingsDir):
            settingFormats = {
                "FLiNGTSettings.ini": "OnLoadMusic = {}",
                "TrainerSettings.ini": "OnLoadMusic={}"
            }
            settingValue = "False" if removeBgMusic else "True"

            for fileName, settingFormat in settingFormats.items():
                settingPath = os.path.join(settingsDir, fileName)
                if not os.path.isfile(settingPath):
                    continue

                with open(settingPath, "r", encoding="utf-8", newline="") as file:
                    lines = file.readlines()

                with open(settingPath, "w", encoding="utf-8", newline="") as file:
                    for line in lines:
                        if line.strip().startswith("OnLoadMusic"):
                            if line.endswith("\r\n"):
                                lineEnding = "\r\n"
                            elif line.endswith("\n"):
                                lineEnding = "\n"
                            else:
                                lineEnding = ""
                            file.write(settingFormat.format(settingValue) + lineEnding)
                        else:
                            file.write(line)

        userProfile = os.environ.get("USERPROFILE", os.path.expanduser("~"))
        localAppData = os.environ.get(
            "LOCALAPPDATA",
            os.path.join(userProfile, "AppData", "Local")
        )
        flingSettingsDirs = [
            os.path.join(userProfile, "Documents", "FLiNGTrainer"),
            os.path.join(localAppData, "FLiNGTrainer")
        ]

        for settingsDir in flingSettingsDirs:
            modify_midi(settingsDir)
            modify_ini_files(settingsDir)

    def remove_bgMusic(self, source_exe):
        LOAD_LIBRARY_AS_DATAFILE = 0x00000002
        LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x00000020
        ERROR_RESOURCE_TYPE_NOT_FOUND = 1813
        resourceTypes = ("MID", "MIDI")

        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        enumNameCallbackType = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM)
        enumLanguageCallbackType = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p, wintypes.WORD, wintypes.LPARAM)

        kernel32.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
        kernel32.LoadLibraryExW.restype = wintypes.HMODULE
        kernel32.FreeLibrary.argtypes = [wintypes.HMODULE]
        kernel32.FreeLibrary.restype = wintypes.BOOL
        kernel32.EnumResourceNamesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, enumNameCallbackType, wintypes.LPARAM]
        kernel32.EnumResourceNamesW.restype = wintypes.BOOL
        kernel32.EnumResourceLanguagesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p, enumLanguageCallbackType, wintypes.LPARAM]
        kernel32.EnumResourceLanguagesW.restype = wintypes.BOOL
        kernel32.BeginUpdateResourceW.argtypes = [wintypes.LPCWSTR, wintypes.BOOL]
        kernel32.BeginUpdateResourceW.restype = wintypes.HANDLE
        kernel32.UpdateResourceW.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, wintypes.WORD, ctypes.c_void_p, wintypes.DWORD]
        kernel32.UpdateResourceW.restype = wintypes.BOOL
        kernel32.EndUpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.BOOL]
        kernel32.EndUpdateResourceW.restype = wintypes.BOOL

        def resource_value(pointer):
            address = pointer or 0
            if address <= 0xFFFF:
                return address
            return ctypes.wstring_at(address)

        def resource_pointer(value):
            if isinstance(value, int):
                return ctypes.c_void_p(value), None

            stringPointer = ctypes.c_wchar_p(value)
            return ctypes.cast(stringPointer, ctypes.c_void_p), stringPointer

        def find_resources(filePath):
            resources = []
            module = kernel32.LoadLibraryExW(filePath, None, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE)
            if not module:
                raise ctypes.WinError(ctypes.get_last_error())

            try:
                for resourceType in resourceTypes:
                    resourceNames = []

                    @enumNameCallbackType
                    def collect_name(moduleHandle, typePointer, namePointer, parameter):
                        resourceNames.append(resource_value(namePointer))
                        return True

                    typePointer, typeString = resource_pointer(resourceType)
                    ctypes.set_last_error(0)
                    foundNames = kernel32.EnumResourceNamesW(module, typePointer, collect_name, 0)
                    if not foundNames:
                        errorCode = ctypes.get_last_error()
                        if errorCode == ERROR_RESOURCE_TYPE_NOT_FOUND:
                            continue
                        raise ctypes.WinError(errorCode)

                    for resourceName in resourceNames:
                        languages = []

                        @enumLanguageCallbackType
                        def collect_language(moduleHandle, typePointer, namePointer, language, parameter):
                            languages.append(language)
                            return True

                        namePointer, nameString = resource_pointer(resourceName)
                        if not kernel32.EnumResourceLanguagesW(module, typePointer, namePointer, collect_language, 0):
                            raise ctypes.WinError(ctypes.get_last_error())

                        resources.extend((resourceType, resourceName, language) for language in languages)
            finally:
                kernel32.FreeLibrary(module)

            return resources

        sourcePath = os.path.abspath(source_exe)
        resources = find_resources(sourcePath)
        if not resources:
            return False

        with open(emptyMidi_path, "rb") as file:
            emptyMidi = file.read()

        if not self.bgMusicMessageSent:
            self.message.emit(tr("Removing trainer background music..."), None)
            self.bgMusicMessageSent = True

        midiBuffer = ctypes.create_string_buffer(emptyMidi)

        updateHandle = kernel32.BeginUpdateResourceW(sourcePath, False)
        if not updateHandle:
            raise ctypes.WinError(ctypes.get_last_error())

        try:
            for resourceType, resourceName, language in resources:
                typePointer, typeString = resource_pointer(resourceType)
                namePointer, nameString = resource_pointer(resourceName)
                if not kernel32.UpdateResourceW(updateHandle, typePointer, namePointer, language, ctypes.cast(midiBuffer, ctypes.c_void_p), len(emptyMidi)):
                    raise ctypes.WinError(ctypes.get_last_error())
        except Exception:
            kernel32.EndUpdateResourceW(updateHandle, True)
            raise

        if not kernel32.EndUpdateResourceW(updateHandle, False):
            raise ctypes.WinError(ctypes.get_last_error())

        return True

    def download_fling(self, selected_trainer):
        if self.update_entry:
            trainerName_display = selected_trainer["trainer_name"]
            self.message.emit(tr("Updating ") + trainerName_display + "...", None)
        else:
            trainerName_display = self.symbol_replacement(selected_trainer["trainer_name"])
            # Trainer duplication check
            for trainerPath in self.trainers.keys():
                if trainerName_display == os.path.splitext(os.path.basename(trainerPath))[0]:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return False

        # Download trainer
        self.message.emit(tr("Downloading..."), "download")
        try:
            targetUrl = self.get_signed_download_url(selected_trainer["url"], raise_errors=True)
            trainerTemp = self.request_download(targetUrl, DOWNLOAD_TEMP_DIR, raise_errors=True)
            if not trainerTemp:
                raise Exception(tr("Internet request failed."))
            self.expected_source_files = {os.path.abspath(trainerTemp): os.path.getsize(trainerTemp)}

        except Exception as e:
            self.message.emit(self.format_request_error(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Extract compressed file and rename
        if os.path.splitext(trainerTemp)[1].lower() in ARCHIVE_EXTENSIONS:
            self.message.emit(tr("Decompressing..."), None)
            try:
                self.expected_source_files = self.extract_archive(trainerTemp, DOWNLOAD_TEMP_DIR, ("info.txt",))

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

        # Locate extracted .exe file
        extractedTrainerNames = []
        extractedAntiCheatNames = []
        for filename in os.listdir(DOWNLOAD_TEMP_DIR):
            if "trainer" in filename.lower() and filename.lower().endswith(".exe"):
                extractedTrainerNames.append(filename)
            elif filename != os.path.basename(trainerTemp) and filename.lower() != "info.txt":
                extractedAntiCheatNames.append(filename)

        # Install auxiliary files as instructions
        if extractedAntiCheatNames:
            self.instructionDst = os.path.join(self.trainerDownloadPath, trainerName_display, "gcm-instructions")
            for antiCheatFile in extractedAntiCheatNames:
                self.src_dst.append({"src": os.path.join(DOWNLOAD_TEMP_DIR, antiCheatFile), "dst": os.path.join(self.instructionDst, antiCheatFile)})

        # Require at least one trainer executable
        if not extractedTrainerNames:
            print("No trainer executable was found in downloaded files.")
            self.message.emit(tr("Trainer files are missing or incomplete. Your antivirus software may have removed them."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Map each trainer version to its destination folder
        os.makedirs(self.trainerDownloadPath, exist_ok=True)
        if len(extractedTrainerNames) > 1:
            if self.update_entry:
                trainerName_trans = self.translate_trainer(selected_trainer)
                if not trainerName_trans:
                    self.message.emit(tr("Failed to translate, please update translation data."), "failure")
                    self.finished.emit(1)
                    return
                trainerName_display = self.symbol_replacement(trainerName_trans)

            for extractedTrainerName in extractedTrainerNames:
                trainer_details = ""
                if selected_trainer['origin'] == "fling_main":
                    pattern = r'trainer(.*)\.exe'
                    match = re.search(pattern, extractedTrainerName, re.IGNORECASE)
                    if match:
                        trainer_details = match.group(1)
                else:
                    pattern = r"\s+Update.*|\s+v\d+.*"
                    match = re.search(pattern, extractedTrainerName)
                    if match:
                        trainer_details = match.group().replace(" Trainer", "").rstrip(".exe")

                trainer_name = f"{trainerName_display}{trainer_details}"

                source_file = os.path.join(DOWNLOAD_TEMP_DIR, extractedTrainerName)
                destination_file = os.path.join(self.trainerDownloadPath, trainer_name, extractedTrainerName)
                self.src_dst.insert(0, {"src": source_file, "dst": destination_file})

        else:
            source_file = os.path.join(DOWNLOAD_TEMP_DIR, extractedTrainerNames[0])
            destination_file = os.path.join(self.trainerDownloadPath, trainerName_display, extractedTrainerNames[0])
            self.src_dst.insert(0, {"src": source_file, "dst": destination_file})

        # remove fling trainer bg music
        if settings["removeFlingBgMusic"]:
            self.modify_fling_settings(True)
            for item in self.src_dst:
                if item["src"].lower().endswith(".exe"):
                    source = os.path.abspath(item["src"])
                    self.remove_bgMusic(source)
                    self.expected_source_files[source] = os.path.getsize(source)
        else:
            self.modify_fling_settings(False)

        if os.path.basename(trainerTemp) not in extractedTrainerNames:
            try:
                os.remove(trainerTemp)
            except OSError as error:
                print(f"Could not remove extracted archive: {error}")

        return True

    @staticmethod
    def apply_binary_patch(data, patch):
        """Masked search and replace over every occurrence"""
        def parse_pattern(pattern_str):
            """"8B??E8" -> (b"\\x8b\\x00\\xe8", [True, False, True]); `??` marks a wildcard byte."""
            tokens = [pattern_str[i:i+2] for i in range(0, len(pattern_str), 2)]
            values = bytes(0 if token == '??' else int(token, 16) for token in tokens)
            mask = [token != '??' for token in tokens]
            return values, mask

        search, search_mask = parse_pattern(patch['search'])
        replace, replace_mask = parse_pattern(patch['replace'])

        expression = b"".join(re.escape(bytes([byte])) if keep else b"." for byte, keep in zip(search, search_mask))
        patched = bytearray(data)
        for match in re.finditer(expression, data, re.DOTALL):
            for offset, (byte, write) in enumerate(zip(replace, replace_mask)):
                if write:
                    patched[match.start() + offset] = byte

        return bytes(patched)

    def unlock_xiaoxing(self, selected_trainer):
        exe_exclusions = ["flashplayer_22.0.0.210_ax_debug.exe"]
        game_name = selected_trainer['game_name']
        patches_to_apply = []

        if game_name in ["Cyberpunk 2077"]:
            patches_to_apply = [
                {'search': "833D????????000F84????????833D????????000F84????????", 'replace': "833D????????00909090909090833D????????00909090909090"},
                {'search': "833D????????000F84????????BA2E", 'replace': "833D????????00909090909090BA2E"},
                {'search': "833D????????000F85????????48C705", 'replace': "833D????????00E9F70300009048C705"},
                {'search': "833D????????00740D833D????????000F85????????BAAC", 'replace': "833D????????009090833D????????00E9C700000090BAAC"},
                {'search': "833D????????00740D833D????????000F85????????488D", 'replace': "833D????????009090833D????????00E90601000090488D"}
            ]
        elif game_name in ["Final Fantasy XV", "Ho Tu Lo Shu The Books of Dragon", "Xuan-Yuan Sword VII"]:
            patches_to_apply = [
                {'search': "E8????????833D????????000F84????????BA2E040000", 'replace': "??????????90909090909090909090909090??????????"},
                {'search': "8D4A??E8????????833D????????000F84????????", 'replace': "????????????????90909090909090909090909090"}
            ]
        elif game_name in ["GuLong", "Palworld", "Baldur's Gate 3", "Starfield", "Hogwarts Legacy", "Sword and Fairy 7", "Path Of Wuxia", "Elden Ring",
                           "Fate Seeker II", "Final Fantasy VII Remake Intergrade"]:
            patches_to_apply = [
                {'search': "8B??E8??????00833D??????00000F84????0000", 'replace': "8B??E8??????00833D??????0000909090909090"},
                {'search': "833D??????00000F84????????833D????????000F84????????", 'replace': "833D??????0000909090909090833D????????00909090909090"}
            ]
        else:
            return

        self.message.emit(tr("Patching..."), None)
        for item in self.src_dst:
            source_dir = item["src"]
            if os.path.isdir(source_dir):
                temp_contents = os.listdir(source_dir)
                exe_file = next((file for file in temp_contents if os.path.isfile(os.path.join(source_dir, file)) and file.lower().endswith(".exe") and file not in exe_exclusions), None)
            else:
                source_dir, exe_file = os.path.split(source_dir)
                if not exe_file.lower().endswith(".exe") or exe_file in exe_exclusions:
                    exe_file = None

            if exe_file:
                original_file = os.path.join(source_dir, exe_file)
                try:
                    with open(original_file, "rb") as trainer_file:
                        patched_data = trainer_file.read()

                    for i, patch in enumerate(patches_to_apply):
                        print(f"Applying patch {i + 1}/{len(patches_to_apply)} for: {game_name}")
                        patched_data = self.apply_binary_patch(patched_data, patch)

                    # Written only once every patch succeeded, so a failure leaves the file as is
                    with open(original_file, "wb") as trainer_file:
                        trainer_file.write(patched_data)
                    print(f"Successfully applied all patches to: {exe_file}")

                except Exception as e:
                    print(f"An error occurred during XiaoXing patching: {e}")

    def download_xiaoxing(self, selected_trainer):
        if self.update_entry:
            trainerName_display = selected_trainer["trainer_name"]
            self.message.emit(tr("Updating ") + trainerName_display + "...", None)
        else:
            trainerName_display = self.symbol_replacement(selected_trainer["trainer_name"])
            # Trainer duplication check
            for trainerPath in self.trainers.keys():
                if self.symbol_replacement(selected_trainer["trainer_name"]) == os.path.splitext(os.path.basename(trainerPath))[0]:
                    self.message.emit(tr("Trainer already exists, aborted download."), "failure")
                    time.sleep(self.download_finish_delay)
                    self.finished.emit(1)
                    return False

        self.message.emit(tr("Downloading..."), "download")
        extractedContentPath = os.path.join(DOWNLOAD_TEMP_DIR, "extracted")
        try:
            signed_url = self.get_signed_download_url(selected_trainer['url'], raise_errors=True)
            trainerTemp = self.request_download(signed_url, DOWNLOAD_TEMP_DIR, raise_errors=True)
            if not trainerTemp:
                raise Exception(tr("Internet request failed."))
            self.expected_source_files = {os.path.abspath(trainerTemp): os.path.getsize(trainerTemp)}

        except Exception as e:
            self.message.emit(self.format_request_error(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Extract compressed file if not single exe
        extracted = False
        if os.path.splitext(trainerTemp)[1].lower() in ARCHIVE_EXTENSIONS:
            extracted = True
            self.message.emit(tr("Decompressing..."), None)
            try:
                self.expected_source_files = self.extract_archive(trainerTemp, extractedContentPath)

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

            try:
                os.remove(trainerTemp)
            except OSError as error:
                print(f"Could not remove extracted archive: {error}")

        if extracted:
            # If the archive contains multiple version folders, split them up into multiple dest folders
            if not self.handle_multi_version_archive(extractedContentPath, trainerName_display, selected_trainer):
                destination_path = os.path.join(self.trainerDownloadPath, trainerName_display)
                self.src_dst.append({"src": extractedContentPath, "dst": destination_path})
        else:
            destination_path = os.path.join(self.trainerDownloadPath, trainerName_display)
            self.src_dst.append({"src": trainerTemp, "dst": os.path.join(destination_path, os.path.basename(trainerTemp))})

        if settings["unlockXiaoXing"]:
            self.unlock_xiaoxing(selected_trainer)

        return True

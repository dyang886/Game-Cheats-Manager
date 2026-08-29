import ctypes
import os
import re
import shutil
import stat
import subprocess
import time
import traceback
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
            except Exception as e:
                self.message.emit(tr("Could not initialize the temporary download folder, please try turning your antivirus software off."), "failure")
                time.sleep(self.update_error_delay)
                self.finished.emit(1)
                return

            self.src_dst = []  # List content: { "src": source_path, "dst": destination_path, "version": YYYY.MM.DD }
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

            try:
                for item in self.src_dst:
                    if os.path.exists(item["dst"]):
                        os.chmod(item["dst"], stat.S_IWRITE)
                    if os.path.isfile(item['src']):
                        dst_dir = os.path.dirname(item["dst"])
                        os.makedirs(dst_dir, exist_ok=True)
                    else:
                        dst_dir = item["dst"]
                    shutil.move(item["src"], item["dst"])

                    if dst_dir != self.instructionDst:
                        info_dict = {
                            "game_name": selected_trainer["game_name"],
                            "origin": selected_trainer["origin"]
                        }
                        if selected_trainer.get("version"):
                            info_dict["version"] = selected_trainer["version"]
                        if selected_trainer["origin"] in ["other", "ct_other"]:
                            info_dict["gcm_url"] = selected_trainer["url"]
                        if selected_trainer.get("extension"):
                            info_dict["extension"] = selected_trainer["extension"]

                        with open(os.path.join(dst_dir, "gcm_info.json"), "w", encoding="utf-8") as info_file:
                            json.dump(info_dict, info_file, ensure_ascii=False, indent=4)

                if self.instructionDst and not self.update_entry:
                    self.messageBox.emit("info", tr("Attention"), tr("This trainer requires additional setup before use. Please check the opened folder for instructions.\nThe instructions are always stored in the 'gcm-instructions' folder."))
                    os.startfile(self.instructionDst)

            except PermissionError as e:
                self.message.emit(tr("Trainer is currently in use, please close any programs using the file and try again."), "failure")
                time.sleep(self.update_error_delay)
                self.finished.emit(1)
                return
            except Exception as e:
                self.message.emit(tr("An error occurred when moving trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return

            if result:
                if self.is_cheat_engine_package(selected_trainer):
                    self.report_cheat_engine_install()
                self.installed.emit(self.installed_trainer_names())
                self.message.emit(tr("Download success!"), "success")
                time.sleep(self.download_finish_delay)
                self.finished.emit(0)

        except Exception as e:
            traceback.print_exc()
            self.message.emit(tr("An error occurred while downloading trainer: ") + str(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)

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
                command = [unzip_path, "x", "-y", trainerTemp, f"-o{extractedContentPath}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

            os.remove(trainerTemp)

        if self.update_entry:
            shutil.rmtree(selected_trainer['trainer_dir'])

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

        except Exception as e:
            self.message.emit(self.format_request_error(e), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Extract compressed file and rename
        if os.path.splitext(trainerTemp)[1].lower() in ARCHIVE_EXTENSIONS:
            self.message.emit(tr("Decompressing..."), None)
            try:
                command = [unzip_path, "x", "-y", trainerTemp, f"-o{DOWNLOAD_TEMP_DIR}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

        # Locate extracted .exe file
        cnt = 0
        extractedTrainerNames = []
        extractedAntiCheatNames = []
        for filename in os.listdir(DOWNLOAD_TEMP_DIR):
            if "trainer" in filename.lower() and filename.endswith(".exe"):
                extractedTrainerNames.append(filename)
            # Count anti-cheat files
            elif ("trainer" not in filename.lower() and filename != os.path.basename(trainerTemp)) and filename.lower() != "info.txt":
                extractedAntiCheatNames.append(filename)
                cnt += 1

        # Warn user if anti-cheat files found
        if cnt > 0:
            self.instructionDst = os.path.join(self.trainerDownloadPath, trainerName_display, "gcm-instructions")
            for antiCheatFile in extractedAntiCheatNames:
                self.src_dst.append({"src": os.path.join(DOWNLOAD_TEMP_DIR, antiCheatFile), "dst": os.path.join(self.instructionDst, antiCheatFile)})

        # Check if extracted trainer name is None
        if not extractedTrainerNames:
            self.message.emit(tr("Could not find the downloaded trainer file, please try turning your antivirus software off."), "failure")
            time.sleep(self.download_finish_delay)
            self.finished.emit(1)
            return False

        # Construct destination trainer name dict (may have multiple versions of a same game)
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
                    self.remove_bgMusic(item["src"])
        else:
            self.modify_fling_settings(False)

        # Delete original trainer file (could not preserve original file name due to multiple versions when updating)
        if self.update_entry:
            shutil.rmtree(selected_trainer['trainer_dir'])

        if os.path.exists(trainerTemp) and os.path.basename(trainerTemp) not in extractedTrainerNames:
            os.remove(trainerTemp)

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
                    print(f"Successfully applied all patches to: {exe_file}\n")

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
                command = [unzip_path, "x", "-y", trainerTemp, f"-o{extractedContentPath}"]
                subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)

            except Exception as e:
                self.message.emit(tr("An error occurred while extracting downloaded trainer: ") + str(e), "failure")
                time.sleep(self.download_finish_delay)
                self.finished.emit(1)
                return False

            os.remove(trainerTemp)

        if self.update_entry:
            shutil.rmtree(selected_trainer['trainer_dir'])

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

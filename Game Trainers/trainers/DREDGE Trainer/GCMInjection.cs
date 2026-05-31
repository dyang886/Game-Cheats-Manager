using UnityEngine;
using UnityEngine.Localization.Settings;
using UnityEngine.Localization;
using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Text;

public class MainThreadDispatcher : MonoBehaviour
{
    private static MainThreadDispatcher instance;
    private Queue<Action> actionQueue = new Queue<Action>();

    private void Awake()
    {
        if (instance != null)
        {
            Destroy(this);
            return;
        }
        instance = this;
        DontDestroyOnLoad(gameObject);
    }

    private void Update()
    {
        lock (actionQueue)
        {
            while (actionQueue.Count > 0)
            {
                Action action = actionQueue.Dequeue();
                try
                {
                    action.Invoke();
                }
                catch (Exception ex)
                {
                    GCMInjection.LogException("MainThreadDispatcher.Update action failed", ex);
                }
            }
        }
    }

    public static void Enqueue(Action action)
    {
        if (instance != null)
        {
            lock (instance.actionQueue)
            {
                instance.actionQueue.Enqueue(action);
            }
        }
        else
        {
            GCMInjection.Log("MainThreadDispatcher.Enqueue failed because dispatcher is missing.");
        }
    }
}

public static class GCMInjection
{
    [DllImport("MonoBridge.dll")]
    private static extern void SendData(string message);

    [DllImport("MonoBridge.dll")]
    private static extern void SendResponse(string message);

    public static void Log(string message)
    {
        string formattedMessage = "[GCMInjection] " + message;
        SendData(formattedMessage);
    }

    public static void LogException(string message, Exception ex)
    {
        Log(message + ": " + ex);
    }

    private static bool SetBooleanMember(object target, string propertyName, string backingFieldName, bool value)
    {
        if (target == null)
        {
            return false;
        }

        Type type = target.GetType();
        PropertyInfo property = type.GetProperty(propertyName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
        MethodInfo setter = property != null ? property.GetSetMethod(true) : null;
        if (setter != null)
        {
            setter.Invoke(target, new object[] { value });
            return true;
        }

        FieldInfo field = type.GetField(backingFieldName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
        if (field != null)
        {
            field.SetValue(target, value);
            return true;
        }

        return false;
    }

    public static void Initialize()
    {
        GameObject existing = GameObject.Find("MainThreadDispatcher");
        if (existing != null)
        {
            return;
        }

        GameObject go = new GameObject("MainThreadDispatcher");
        go.AddComponent<MainThreadDispatcher>();
    }

    public static void SetGodMode(bool enabled)
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            Player player = GameManager.Instance != null ? GameManager.Instance.Player : null;
            if (player == null)
            {
                Log("SetGodMode failed: player is unavailable.");
                return;
            }

            bool setGod = SetBooleanMember(player, "IsGodModeEnabled", "<IsGodModeEnabled>k__BackingField", enabled);
            bool setImmune = SetBooleanMember(player, "IsImmuneModeEnabled", "<IsImmuneModeEnabled>k__BackingField", enabled);
            if (!setGod || !setImmune)
            {
                Log("SetGodMode failed: player god/immune fields were not found.");
            }
        });
    }

    public static void SpawnItem(int index)
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            int i = 0;
            foreach (ItemData itemData in GameManager.Instance.ItemManager.allItems)
            {
                if (i == index)
                {
                    SpatialItemInstance spatialItemInstance = GameManager.Instance.ItemManager.CreateItem<SpatialItemInstance>(itemData);
                    Vector3Int position;
                    if (GameManager.Instance.SaveData.Inventory.FindPositionForObject(spatialItemInstance.GetItemData<SpatialItemData>(), out position, 0, false))
                    {
                        GameManager.Instance.ItemManager.AddObjectToInventory(spatialItemInstance, position, true);
                    }
                }
                i++;
            }
        });
    }

    public static void AddFunds(float amount)
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            decimal decimalAmount = new decimal(amount);
            GameManager.Instance.AddFunds(decimalAmount);
        });
    }

    public static void RepairAll()
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            GameManager.Instance.ItemManager.RepairAllItemDurability();
            GameManager.Instance.ItemManager.RepairHullDamage(true);
        });
    }

    public static void RestockAll()
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            HarvestPOI[] POIs = UnityEngine.Object.FindObjectsOfType<HarvestPOI>();
            foreach (HarvestPOI harvest in POIs)
            {
                harvest.AddStock(harvest.MaxStock, true);
            }
        });
    }

    public static void ClearWeather()
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            WeatherController weather = GameManager.Instance.WeatherController;
            weather.ChangeWeather("clear");
        });
    }

    public static void FreezeTime(int freeze)
    {
        bool enabled = freeze != 0;
        MainThreadDispatcher.Enqueue(() =>
        {
            GameManager.Instance.Time.ToggleFreezeTime(enabled);
        });
    }

    public static void GetItemList()
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            StringBuilder sb = new StringBuilder();
            int i = 0;
            foreach (ItemData itemData in GameManager.Instance.ItemManager.allItems)
            {
                string localizedString = LocalizationSettings.StringDatabase.GetLocalizedString(itemData.itemNameKey.TableReference, itemData.itemNameKey.TableEntryReference, null, FallbackBehavior.UseProjectSettings, Array.Empty<object>());
                sb.AppendLine(i.ToString() + ">" + localizedString);
                i++;
            }
            SendResponse(sb.ToString());
        });
    }
}

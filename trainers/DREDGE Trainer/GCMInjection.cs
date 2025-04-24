using UnityEngine;
using System.IO;
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
                actionQueue.Dequeue().Invoke();
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
    }
}
public static class GCMInjection
{
    [DllImport("MonoBridge.dll")]
    private static extern void SendData(string message);

    private static void Log(string message) {
        string formattedMessage = "[GCMInjection] " + message;
        SendData(formattedMessage);
    }

    public static void Initialize()
    {
        GameObject go = new GameObject("MainThreadDispatcher");
        go.AddComponent<MainThreadDispatcher>();
    }

    public static void GodMode(bool enable)
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            Player player = GameManager.Instance.Player;
            if (player != null)
            {
                player.IsImmuneModeEnabled = enable;
                FieldInfo godModeField = typeof(Player).GetField("IsGodModeEnabled", BindingFlags.NonPublic | BindingFlags.Instance);
                if (godModeField != null)
                {
                    godModeField.SetValue(player, enable);
                }
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

    public static void FreezeTime(bool freeze)
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            GameManager.Instance.Time.ToggleFreezeTime(freeze);
        });
    }

    public static void Dump()
    {
        MainThreadDispatcher.Enqueue(() =>
        {
            StringBuilder sb = new StringBuilder();
            int i = 0;
            foreach (ItemData itemData in GameManager.Instance.ItemManager.allItems)
            {
                string localizedString = LocalizationSettings.StringDatabase.GetLocalizedString(itemData.itemNameKey.TableReference, itemData.itemNameKey.TableEntryReference, null, FallbackBehavior.UseProjectSettings, Array.Empty<object>());
                sb.AppendLine(i.ToString() + ":" + localizedString);
                i++;
            }
            Log(sb.ToString());
        });
    }
}
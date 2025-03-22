using UnityEngine;
using System.IO;
using UnityEngine.Localization.Settings;
using UnityEngine.Localization;
using System;

public class GCMInjection
{
    public static void SpawnItem(int index)
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
            i = i + 1;
        }
    }
}
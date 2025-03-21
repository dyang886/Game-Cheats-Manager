using UnityEngine;

public class GCMInjection
{
    public static void SpawnItem(int iindex)
    {
        int i = 0;
        foreach (ItemData itemData in GameManager.Instance.ItemManager.allItems)
        {
            if (i == iindex)
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
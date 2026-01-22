// =============================================================
// ARGH_AmbientVehiclePrefabBootstrapComponent.c
//
// Attach this component to your AmbientVehicleSpawnpoint_*.et prefabs.
// It forces a refresh after init (helps prefab-preview + consistent init).
// Optional: triggers catalog rebuild in WORKBENCH.
// =============================================================

class ARGH_AmbientVehiclePrefabBootstrapComponentClass : ScriptComponentClass {}

class ARGH_AmbientVehiclePrefabBootstrapComponent : ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Force refresh (runs Update selection) after init")]
	bool m_bRefreshOnInit;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "WORKBENCH: trigger catalog rebuild when prefab is loaded")]
	bool m_bRebuildCatalogInWorkbench;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

#ifdef WORKBENCH
		if (m_bRebuildCatalogInWorkbench)
			SCR_AmbientVehicleSpawnPointComponent.TriggerCatalogRebuild();
#endif

		if (!m_bRefreshOnInit)
			return;

		if (!GetGame())
			return;

		SCR_AmbientVehicleSpawnPointComponent sp = SCR_AmbientVehicleSpawnPointComponent.Cast(GetOwner().FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!sp)
			return;

		// Uses your public helper so we can call the protected Update safely
		sp.ARGH_RefreshNow();
	}
}

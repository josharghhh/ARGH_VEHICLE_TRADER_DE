// ARGH_VehicleDealerConfig.c
// Dealer configuration supporting ambient catalog imports with price overrides.

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sDisplayName")]
class ARGH_VehicleForSale : Managed
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Vehicle prefab", category: "Vehicle")]
	ResourceName m_sPrefab;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name", category: "Vehicle")]
	string m_sDisplayName;

	[Attribute(defvalue: "Civilian", uiwidget: UIWidgets.EditBox, desc: "Category label", category: "Vehicle")]
	string m_sCategory;

	[Attribute(defvalue: "5000", uiwidget: UIWidgets.EditBox, desc: "Price", category: "Vehicle")]
	int m_iPrice;

	[Attribute(defvalue: "4", uiwidget: UIWidgets.EditBox, desc: "Seat count", category: "Vehicle")]
	int m_iSeats;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Requires license", category: "Vehicle")]
	bool m_bRequiresLicense;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "edds", desc: "Thumbnail image (edds)", category: "UI")]
	ResourceName m_sThumbnailPath;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enable vehicle", category: "Vehicle")]
	bool m_bEnabled;

	bool IsEnabled()
	{
		return m_bEnabled && !m_sPrefab.IsEmpty();
	}

	string GetDisplayName()
	{
		if (!m_sDisplayName.IsEmpty())
			return m_sDisplayName;
		return ARGH_VehicleDealerConfig.ExtractDisplayNameFromPrefab(m_sPrefab);
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sPrefab")]
class ARGH_VehiclePriceOverride : Managed
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Vehicle prefab to override", category: "Override")]
	ResourceName m_sPrefab;

	[Attribute(defvalue: "5000", uiwidget: UIWidgets.EditBox, desc: "Price override", category: "Override")]
	int m_iPrice;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Category override", category: "Override")]
	string m_sCategory;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name override", category: "Override")]
	string m_sDisplayName;

	[Attribute(defvalue: "4", uiwidget: UIWidgets.EditBox, desc: "Seat override", category: "Override")]
	int m_iSeats;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Requires license override", category: "Override")]
	bool m_bRequiresLicense;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "edds", desc: "Thumbnail override (edds)", category: "Override")]
	ResourceName m_sThumbnailPath;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enable override", category: "Override")]
	bool m_bEnabled;
}

[BaseContainerProps(configRoot: true)]
class ARGH_VehicleDealerConfig : Managed
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Dealer ID", category: "Identity")]
	string m_sDealerId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Dealer display name", category: "Identity")]
	string m_sDealerName;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "conf", desc: "Ambient vehicle catalog to import", category: "Catalog")]
	ResourceName m_rAmbientCatalog;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Import ambient vehicle catalog", category: "Catalog")]
	bool m_bUseAmbientCatalog;

	[Attribute(defvalue: "5000", uiwidget: UIWidgets.EditBox, desc: "Default price for imported vehicles", category: "Catalog")]
	int m_iDefaultPrice;

	[Attribute(defvalue: "{}", desc: "Explicit vehicle entries (override or append)", category: "Catalog")]
	ref array<ref ARGH_VehicleForSale> m_aVehicles;

	[Attribute(defvalue: "{}", desc: "Price/category overrides by prefab", category: "Catalog")]
	ref array<ref ARGH_VehiclePriceOverride> m_aPriceOverrides;

	protected ref array<ref ARGH_VehicleForSale> m_aResolvedVehicles;
	protected bool m_bResolved;

	//------------------------------------------------------------------------------------------------
	static string ExtractDisplayNameFromPrefab(ResourceName prefab)
	{
		string path = prefab;
		path.Replace("\\", "/");

		int slash = path.LastIndexOf("/");
		if (slash >= 0)
			path = path.Substring(slash + 1, path.Length() - (slash + 1));

		int dot = path.LastIndexOf(".");
		if (dot > 0)
			path = path.Substring(0, dot);

		return path;
	}

	//------------------------------------------------------------------------------------------------
	int GetEnabledVehicleCount()
	{
		array<ref ARGH_VehicleForSale> vehicles = GetEnabledVehicles();
		if (!vehicles)
			return 0;
		return vehicles.Count();
	}

	//------------------------------------------------------------------------------------------------
	array<ref ARGH_VehicleForSale> GetEnabledVehicles()
	{
		EnsureResolved();

		array<ref ARGH_VehicleForSale> enabled = new array<ref ARGH_VehicleForSale>();
		if (!m_aResolvedVehicles)
			return enabled;

		foreach (ARGH_VehicleForSale v : m_aResolvedVehicles)
		{
			if (v && v.IsEnabled())
				enabled.Insert(v);
		}

		return enabled;
	}

	//------------------------------------------------------------------------------------------------
	ARGH_VehicleForSale FindVehicleByPrefab(ResourceName prefab)
	{
		EnsureResolved();
		if (!m_aResolvedVehicles)
			return null;

		foreach (ARGH_VehicleForSale v : m_aResolvedVehicles)
		{
			if (v && v.m_sPrefab == prefab)
				return v;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	int GetVehiclePrice(ResourceName prefab)
	{
		ARGH_VehicleForSale vehicle = FindVehicleByPrefab(prefab);
		if (!vehicle)
			return -1;
		return vehicle.m_iPrice;
	}

	//------------------------------------------------------------------------------------------------
	protected void EnsureResolved()
	{
		if (m_bResolved)
			return;

		m_aResolvedVehicles = new array<ref ARGH_VehicleForSale>();
		m_bResolved = true;

		map<string, ref ARGH_VehicleForSale> byPrefab = new map<string, ref ARGH_VehicleForSale>();

		if (m_bUseAmbientCatalog && !m_rAmbientCatalog.IsEmpty())
			LoadAmbientCatalog(byPrefab);

		if (m_aVehicles)
		{
			foreach (ARGH_VehicleForSale v : m_aVehicles)
			{
				if (!v || v.m_sPrefab.IsEmpty())
					continue;

				byPrefab.Set(v.m_sPrefab, v);
			}
		}

		foreach (string key, ARGH_VehicleForSale value : byPrefab)
		{
			if (value)
				m_aResolvedVehicles.Insert(value);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadAmbientCatalog(map<string, ref ARGH_VehicleForSale> outByPrefab)
	{
		Resource res = Resource.Load(m_rAmbientCatalog);
		if (!res || !res.IsValid())
			return;

		BaseContainer bc = res.GetResource().ToBaseContainer();
		if (!bc)
			return;

		string catalogPath = m_rAmbientCatalog.GetPath();
		string catalogPathLower = catalogPath;
		catalogPathLower.ToLower();
		bool isFlyingCatalog = catalogPathLower.Contains("flying");

		array<ref ARGH_AmbientVehicleSpawnRule> flatRules = {};
		array<ref ARGH_AmbientFlyingVehicleSpawnRule> flatFlyingRules = {};

		if (!isFlyingCatalog)
		{
			array<ref ARGH_AmbientVehicleGroup> groups = {};
			array<ref ARGH_AmbientVehicleSpawnRule> rules = {};
			bool hasGroups = bc.Get("m_aGroups", groups);
			bool hasRules = bc.Get("m_aVehicles", rules);

			if (hasGroups && groups.Count() > 0)
				FlattenAmbientGroups(groups, flatRules, true);
			else if (hasRules && rules.Count() > 0)
				flatRules = rules;
		}
		else
		{
			array<ref ARGH_AmbientFlyingVehicleGroup> flyingGroups = {};
			array<ref ARGH_AmbientFlyingVehicleSpawnRule> flyingRules = {};
			bool hasFlyingGroups = bc.Get("m_aGroups", flyingGroups);
			bool hasFlyingRules = bc.Get("m_aVehicles", flyingRules);

			if (hasFlyingGroups && flyingGroups.Count() > 0)
				FlattenAmbientFlyingGroups(flyingGroups, flatFlyingRules, true);
			else if (hasFlyingRules && flyingRules.Count() > 0)
				flatFlyingRules = flyingRules;
		}

		map<string, ref ARGH_VehiclePriceOverride> overrides = BuildOverrideMap();

		foreach (ARGH_AmbientVehicleSpawnRule rule : flatRules)
		{
			if (!rule || !rule.m_bEnabled || rule.m_fSpawnWeight <= 0.0)
				continue;

			ARGH_VehicleForSale v = new ARGH_VehicleForSale();
			v.m_sPrefab = rule.m_rPrefab;
			v.m_sDisplayName = rule.m_sDisplayName;
			v.m_sCategory = "Civilian";
			if (rule.m_iSupplyCost > 0)
				v.m_iPrice = rule.m_iSupplyCost;
			else
				v.m_iPrice = m_iDefaultPrice;
			v.m_iSeats = 4;
			v.m_bRequiresLicense = false;
			v.m_sThumbnailPath = string.Empty;
			v.m_bEnabled = true;

			ApplyOverride(v, overrides);
			outByPrefab.Set(v.m_sPrefab, v);
		}

		foreach (ARGH_AmbientFlyingVehicleSpawnRule rule : flatFlyingRules)
		{
			if (!rule || !rule.m_bEnabled || rule.m_fSpawnWeight <= 0.0)
				continue;

			ARGH_VehicleForSale v = new ARGH_VehicleForSale();
			v.m_sPrefab = rule.m_rPrefab;
			v.m_sDisplayName = rule.m_sDisplayName;
			v.m_sCategory = "Civilian";
			if (rule.m_iSupplyCost > 0)
				v.m_iPrice = rule.m_iSupplyCost;
			else
				v.m_iPrice = m_iDefaultPrice;
			v.m_iSeats = 4;
			v.m_bRequiresLicense = false;
			v.m_sThumbnailPath = string.Empty;
			v.m_bEnabled = true;

			ApplyOverride(v, overrides);
			outByPrefab.Set(v.m_sPrefab, v);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected map<string, ref ARGH_VehiclePriceOverride> BuildOverrideMap()
	{
		map<string, ref ARGH_VehiclePriceOverride> result = new map<string, ref ARGH_VehiclePriceOverride>();
		if (!m_aPriceOverrides)
			return result;

		foreach (ARGH_VehiclePriceOverride o : m_aPriceOverrides)
		{
			if (!o || o.m_sPrefab.IsEmpty())
				continue;
			result.Set(o.m_sPrefab, o);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyOverride(ARGH_VehicleForSale v, map<string, ref ARGH_VehiclePriceOverride> overrides)
	{
		if (!v || !overrides)
			return;

		ARGH_VehiclePriceOverride o;
		if (!overrides.Find(v.m_sPrefab, o) || !o)
			return;

		v.m_iPrice = o.m_iPrice;
		if (!o.m_sCategory.IsEmpty())
			v.m_sCategory = o.m_sCategory;
		if (!o.m_sDisplayName.IsEmpty())
			v.m_sDisplayName = o.m_sDisplayName;
		v.m_iSeats = o.m_iSeats;
		v.m_bRequiresLicense = o.m_bRequiresLicense;
		if (!o.m_sThumbnailPath.IsEmpty())
			v.m_sThumbnailPath = o.m_sThumbnailPath;
		v.m_bEnabled = o.m_bEnabled;
	}

	//------------------------------------------------------------------------------------------------
	protected void FlattenAmbientGroups(array<ref ARGH_AmbientVehicleGroup> groups, array<ref ARGH_AmbientVehicleSpawnRule> outRules, bool parentEnabled)
	{
		if (!groups || !outRules)
			return;

		foreach (ARGH_AmbientVehicleGroup g : groups)
		{
			if (!g)
				continue;

			bool enabled = parentEnabled && g.m_bEnabled;
			if (!enabled)
				continue;

			if (g.m_aVehicles)
			{
				foreach (ARGH_AmbientVehicleSpawnRule r : g.m_aVehicles)
				{
					if (!r)
						continue;
					outRules.Insert(r);
				}
			}

			if (g.m_aChildren)
				FlattenAmbientGroups(g.m_aChildren, outRules, enabled);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void FlattenAmbientFlyingGroups(array<ref ARGH_AmbientFlyingVehicleGroup> groups, array<ref ARGH_AmbientFlyingVehicleSpawnRule> outRules, bool parentEnabled)
	{
		if (!groups || !outRules)
			return;

		foreach (ARGH_AmbientFlyingVehicleGroup g : groups)
		{
			if (!g)
				continue;

			bool enabled = parentEnabled && g.m_bEnabled;
			if (!enabled)
				continue;

			if (g.m_aVehicles)
			{
				foreach (ARGH_AmbientFlyingVehicleSpawnRule r : g.m_aVehicles)
				{
					if (!r)
						continue;
					outRules.Insert(r);
				}
			}

			if (g.m_aChildren)
				FlattenAmbientFlyingGroups(g.m_aChildren, outRules, enabled);
		}
	}
}

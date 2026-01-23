// ARGH_VehicleCardWidget.c
// Widget component representing a single vehicle option in the vehicle spawner grid.
// Shows thumbnail, name, and availability status.

// Vehicle category enum for filtering
enum ARGH_EVehicleCategory
{
	ALL = 0,
	POLICE_PATROL = 1,
	POLICE_SUV = 2,
	SHERIFF = 3,
	UNMARKED = 4,
	FIRE = 5,
	EMS = 6
}

// Vehicle data class for UI display
class ARGH_VehicleCardData
{
	ResourceName m_sPrefab;
	string m_sDisplayName;
	ARGH_EVehicleCategory m_eCategory;
	bool m_bAvailable;
	int m_iIndex;
	
	void ARGH_VehicleCardData() {}
	
	//------------------------------------------------------------------------------------------------
	static ARGH_VehicleCardData Create(ResourceName prefab, string displayName, ARGH_EVehicleCategory category, int index, bool available = true)
	{
		ARGH_VehicleCardData data = new ARGH_VehicleCardData();
		data.m_sPrefab = prefab;
		data.m_sDisplayName = displayName;
		data.m_eCategory = category;
		data.m_iIndex = index;
		data.m_bAvailable = available;
		return data;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get category display name
	static string GetCategoryName(ARGH_EVehicleCategory category)
	{
		switch (category)
		{
			case ARGH_EVehicleCategory.ALL:
				return "All Vehicles";
			case ARGH_EVehicleCategory.POLICE_PATROL:
				return "Police Patrol";
			case ARGH_EVehicleCategory.POLICE_SUV:
				return "Police SUV";
			case ARGH_EVehicleCategory.SHERIFF:
				return "Sheriff";
			case ARGH_EVehicleCategory.UNMARKED:
				return "Unmarked";
			case ARGH_EVehicleCategory.FIRE:
				return "Fire Dept";
			case ARGH_EVehicleCategory.EMS:
				return "EMS";
		}
		return "Unknown";
	}
	
	//------------------------------------------------------------------------------------------------
	//! Determine category from prefab path (heuristic)
	static ARGH_EVehicleCategory DetermineCategoryFromPrefab(ResourceName prefab)
	{
		string path = prefab;
		path.ToLower();
		
		if (path.Contains("fire") || path.Contains("firetruck") || path.Contains("engine"))
		{
			return ARGH_EVehicleCategory.FIRE;
		}
		if (path.Contains("ems") || path.Contains("ambulance") || path.Contains("medic"))
		{
			return ARGH_EVehicleCategory.EMS;
		}
		if (path.Contains("sheriff"))
		{
			return ARGH_EVehicleCategory.SHERIFF;
		}
		if (path.Contains("unmarked") || path.Contains("detective") || path.Contains("undercover"))
		{
			return ARGH_EVehicleCategory.UNMARKED;
		}
		if (path.Contains("suv") || path.Contains("tahoe") || path.Contains("explorer"))
		{
			return ARGH_EVehicleCategory.POLICE_SUV;
		}
		if (path.Contains("police") || path.Contains("patrol") || path.Contains("cruiser") || path.Contains("charger") || path.Contains("crown"))
		{
			return ARGH_EVehicleCategory.POLICE_PATROL;
		}
		
		// Default to patrol
		return ARGH_EVehicleCategory.POLICE_PATROL;
	}
}

// Event handler for vehicle card interactions
class ARGH_VehicleCardWidgetHandler : ScriptedWidgetEventHandler
{
	protected ARGH_VehicleCardWidget m_pCard;
	
	//------------------------------------------------------------------------------------------------
	void SetCard(ARGH_VehicleCardWidget card)
	{
		m_pCard = card;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_pCard || button != 0)
			return false;
		
		m_pCard.OnCardClicked();
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_pCard)
			m_pCard.OnCardHover(true);
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_pCard)
			m_pCard.OnCardHover(false);
		return false;
	}
}

// Vehicle card widget component
class ARGH_VehicleCardWidget
{
	// Widget references
	protected Widget m_wRoot;
	protected Widget m_wCardFrame;
	protected ImageWidget m_wBackground;
	protected ImageWidget m_wSelectionBorder;
	protected ImageWidget m_wThumbnail;
	protected TextWidget m_wVehicleName;
	protected Widget m_wAvailabilityBadge;
	protected TextWidget m_wAvailabilityText;
	protected Widget m_wHoverOverlay;
	
	// Handler
	protected ref ARGH_VehicleCardWidgetHandler m_pHandler;
	
	// State
	protected ref ARGH_VehicleCardData m_pData;
	protected bool m_bIsSelected;
	protected bool m_bIsHovered;
	
	// Callback for selection
	protected ref ScriptInvoker m_OnSelected;
	
	// Colors
	protected const int COLOR_NORMAL = 0x80333333;       // Dark semi-transparent
	protected const int COLOR_HOVER = 0xA0444444;        // Lighter on hover
	protected const int COLOR_SELECTED = 0xFF2A5A9E;     // Blue when selected
	protected const int COLOR_UNAVAILABLE = 0x80550000;  // Red-tinted when unavailable
	protected const int COLOR_BORDER_SELECTED = 0xFF4A9AEE;  // Light blue border
	
	//------------------------------------------------------------------------------------------------
	//! Initialize the card with a root widget
	bool Initialize(Widget root)
	{
		if (!root)
			return false;
		
		m_wRoot = root;
		
		// Find child widgets
		m_wCardFrame = m_wRoot.FindAnyWidget("CardFrame");
		m_wBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("CardBackground"));
		m_wSelectionBorder = ImageWidget.Cast(m_wRoot.FindAnyWidget("SelectionBorder"));
		m_wThumbnail = ImageWidget.Cast(m_wRoot.FindAnyWidget("VehicleThumbnail"));
		m_wVehicleName = TextWidget.Cast(m_wRoot.FindAnyWidget("VehicleName"));
		m_wAvailabilityBadge = m_wRoot.FindAnyWidget("AvailabilityBadge");
		m_wAvailabilityText = TextWidget.Cast(m_wRoot.FindAnyWidget("AvailabilityText"));
		m_wHoverOverlay = m_wRoot.FindAnyWidget("HoverOverlay");
		
		// Setup handler
		m_pHandler = new ARGH_VehicleCardWidgetHandler();
		m_pHandler.SetCard(this);
		m_wRoot.AddHandler(m_pHandler);
		
		// Create event invoker
		m_OnSelected = new ScriptInvoker();
		
		m_bIsSelected = false;
		m_bIsHovered = false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set the vehicle data to display
	void SetData(ARGH_VehicleCardData data)
	{
		m_pData = data;
		
		if (!m_pData)
		{
			if (m_wRoot)
				m_wRoot.SetVisible(false);
			return;
		}
		
		m_wRoot.SetVisible(true);
		
		// Update name
		if (m_wVehicleName)
		{
			m_wVehicleName.SetText(data.m_sDisplayName);
		}
		
		// Update availability badge
		if (m_wAvailabilityBadge)
		{
			m_wAvailabilityBadge.SetVisible(!data.m_bAvailable);
		}
		if (m_wAvailabilityText && !data.m_bAvailable)
		{
			m_wAvailabilityText.SetText("Cooldown");
		}
		
		// Update thumbnail (placeholder - would need actual vehicle thumbnails)
		if (m_wThumbnail)
		{
			// For now, just show a placeholder. In the future, you could:
			// 1. Use pre-rendered thumbnails from textures
			// 2. Use the ItemPreviewWidget for each card (expensive)
			// 3. Generate thumbnails dynamically
		}
		
		// Update visual state
		UpdateVisualState();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the vehicle data
	ARGH_VehicleCardData GetData()
	{
		return m_pData;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set selected state
	void SetSelected(bool selected)
	{
		m_bIsSelected = selected;
		UpdateVisualState();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if selected
	bool IsSelected()
	{
		return m_bIsSelected;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called when card is clicked
	void OnCardClicked()
	{
		if (!m_pData)
			return;
		
		// Notify listeners
		if (m_OnSelected)
		{
			m_OnSelected.Invoke(this);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called on hover state change
	void OnCardHover(bool isHovered)
	{
		m_bIsHovered = isHovered;
		UpdateVisualState();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Update visual appearance based on state
	protected void UpdateVisualState()
	{
		if (!m_wBackground)
			return;
		
		int bgColor = COLOR_NORMAL;
		
		if (!m_pData || !m_pData.m_bAvailable)
		{
			bgColor = COLOR_UNAVAILABLE;
		}
		else if (m_bIsSelected)
		{
			bgColor = COLOR_SELECTED;
		}
		else if (m_bIsHovered)
		{
			bgColor = COLOR_HOVER;
		}
		
		m_wBackground.SetColor(Color.FromInt(bgColor));
		
		// Selection border
		if (m_wSelectionBorder)
		{
			m_wSelectionBorder.SetVisible(m_bIsSelected);
			if (m_bIsSelected)
			{
				m_wSelectionBorder.SetColor(Color.FromInt(COLOR_BORDER_SELECTED));
			}
		}
		
		// Hover overlay
		if (m_wHoverOverlay)
		{
			m_wHoverOverlay.SetVisible(m_bIsHovered && !m_bIsSelected);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get selection event invoker
	ScriptInvoker GetOnSelected()
	{
		return m_OnSelected;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get root widget
	Widget GetRootWidget()
	{
		return m_wRoot;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get vehicle index
	int GetVehicleIndex()
	{
		if (m_pData)
			return m_pData.m_iIndex;
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Cleanup
	void Destroy()
	{
		if (m_wRoot && m_pHandler)
		{
			m_wRoot.RemoveHandler(m_pHandler);
		}
		m_pHandler = null;
		m_pData = null;
		m_OnSelected = null;
	}
}

// ARGH_VehicleDealerOverlayUI.c
// Fallback UI when menu presets are unavailable.

class ARGH_VehicleDealerOverlayButtonHandler : ScriptedWidgetEventHandler
{
	protected ARGH_VehicleDealerOverlayUI m_Menu;
	protected string m_Action;

	void Init(ARGH_VehicleDealerOverlayUI menu, string action)
	{
		m_Menu = menu;
		m_Action = action;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Menu || button != 0)
			return false;

		m_Menu.HandleButtonAction(m_Action);
		return true;
	}
}

class ARGH_VehicleDealerOverlayCategoryHandler : ScriptedWidgetEventHandler
{
	protected ARGH_VehicleDealerOverlayUI m_Menu;
	protected string m_Category;

	void Init(ARGH_VehicleDealerOverlayUI menu, string category)
	{
		m_Menu = menu;
		m_Category = category;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Menu || button != 0)
			return false;

		m_Menu.SetCategoryFilter(m_Category);
		return true;
	}
}

class ARGH_VehicleDealerOverlayItemHandler : ScriptedWidgetEventHandler
{
	protected ARGH_VehicleDealerOverlayUI m_Menu;
	protected int m_Index;

	void Init(ARGH_VehicleDealerOverlayUI menu, int index)
	{
		m_Menu = menu;
		m_Index = index;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Menu || button != 0)
			return false;

		m_Menu.SelectVehicleByIndex(m_Index);
		return true;
	}
}

class ARGH_VehicleDealerOverlayUI
{
	protected static ref ARGH_VehicleDealerOverlayUI s_pActive;

	protected CharacterControllerComponent m_PlayerController;
	protected bool m_WasMovementDisabled;
	protected bool m_WasViewDisabled;
	protected bool m_WasWeaponDisabled;
	protected bool m_AppliedSpeedOverride;
	protected bool m_AppliedNoFireOverride;
	protected InputManager m_InputManager;
	protected bool m_InputListenersActive;
	protected SCR_InteractionHandlerComponent m_InteractionHandler;
	protected bool m_WasNearbyCollectionEnabled;
	protected bool m_WasManualCollectionOverride;
	protected bool m_WasManualNearbyOverride;

	protected Widget m_Root;
	protected TextWidget m_TitleText;
	protected TextWidget m_VehicleName;
	protected TextWidget m_VehiclePrice;
	protected TextWidget m_WalletValue;
	protected TextWidget m_BankValue;
	protected TextWidget m_StatusText;
	protected Widget m_CloseButton;
	protected Widget m_BuyButton;
	protected VerticalLayoutWidget m_CategoryList;
	protected VerticalLayoutWidget m_VehicleGrid;

	protected ARGH_VehicleDealerComponent m_Dealer;
	protected ref array<ref ARGH_VehicleForSaleDto> m_Catalog;
	protected ref array<ref ARGH_VehicleForSaleDto> m_Filtered;
	protected ref array<Widget> m_CategoryWidgets;
	protected ref array<Widget> m_ItemWidgets;
	protected ref array<ImageWidget> m_ItemBackgrounds;
	protected ref array<ref ScriptedWidgetEventHandler> m_Handlers;

	protected int m_SelectedIndex = -1;
	protected ResourceName m_SelectedPrefab;
	protected string m_CurrentCategory = "All";

	protected const int COLOR_ITEM_NORMAL = 0xFF0A0B0C;
	protected const int COLOR_ITEM_SELECTED = 0xFF24323A;
	protected const int COLOR_STATUS_OK = 0xFF7CC87C;
	protected const int COLOR_STATUS_WARN = 0xFFC18A4C;
	protected const int COLOR_STATUS_ERROR = 0xFFCE5A4C;

	//------------------------------------------------------------------------------------------------
	static void Toggle(ARGH_VehicleDealerComponent dealer)
	{
		if (s_pActive)
		{
			s_pActive.Close();
			s_pActive = null;
			return;
		}

		ARGH_VehicleDealerOverlayUI ui = new ARGH_VehicleDealerOverlayUI();
		if (!ui.Open(dealer))
		{
			delete ui;
			return;
		}

		s_pActive = ui;
	}

	//------------------------------------------------------------------------------------------------
	bool Open(ARGH_VehicleDealerComponent dealer)
	{
		m_Root = GetGame().GetWorkspace().CreateWidgets("{8CBF75B6F96E412D}UI/ARGH_menu_vehicle_dealer.layout");
		if (!m_Root)
			return false;
		ARGH_VehicleDealerMenuContext.SetMenuOpen(true);

		m_TitleText = TextWidget.Cast(m_Root.FindAnyWidget("TitleText"));
		m_VehicleName = TextWidget.Cast(m_Root.FindAnyWidget("VehicleName"));
		m_VehiclePrice = TextWidget.Cast(m_Root.FindAnyWidget("VehiclePrice"));
		m_WalletValue = TextWidget.Cast(m_Root.FindAnyWidget("WalletValue"));
		m_BankValue = TextWidget.Cast(m_Root.FindAnyWidget("BankValue"));
		m_StatusText = TextWidget.Cast(m_Root.FindAnyWidget("StatusText"));
		m_CloseButton = m_Root.FindAnyWidget("CloseButton");
		m_BuyButton = m_Root.FindAnyWidget("BuyButton");
		m_CategoryList = VerticalLayoutWidget.Cast(m_Root.FindAnyWidget("CategoryList"));
		m_VehicleGrid = VerticalLayoutWidget.Cast(m_Root.FindAnyWidget("VehicleGrid"));

		m_Catalog = new array<ref ARGH_VehicleForSaleDto>();
		m_Filtered = new array<ref ARGH_VehicleForSaleDto>();
		m_CategoryWidgets = new array<Widget>();
		m_ItemWidgets = new array<Widget>();
		m_ItemBackgrounds = new array<ImageWidget>();
		m_Handlers = new array<ref ScriptedWidgetEventHandler>();

		BindButton(m_CloseButton, "close");
		BindButton(m_BuyButton, "buy");
		BindCategoryButton(m_Root.FindAnyWidget("CategoryAll"), "All");

		m_Dealer = dealer;
		if (m_Dealer && m_TitleText)
			m_TitleText.SetText(m_Dealer.GetDealerName());

		if (m_Dealer)
		{
			m_Dealer.GetOnCatalogReceived().Insert(OnCatalogReceived);
			m_Dealer.GetOnPurchaseResult().Insert(OnPurchaseResult);
			m_Dealer.ClientRequestCatalog();
			SetStatus("Loading catalog...", false, false);
		}
		else
		{
			SetStatus("Dealer unavailable.", true, false);
		}

		UpdateBalances();
		GetGame().GetCallqueue().CallLater(UpdateBalances, 1000, true);
		CaptureInput(true);
		GetGame().GetCallqueue().CallLater(UpdateInputState, 16, true);
		RegisterInputListeners();
		FocusMenuRoot();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(UpdateBalances);
		GetGame().GetCallqueue().Remove(UpdateInputState);
		UnregisterInputListeners();
		ARGH_VehicleDealerMenuContext.SetMenuOpen(false);

		if (m_Dealer)
		{
			m_Dealer.GetOnCatalogReceived().Remove(OnCatalogReceived);
			m_Dealer.GetOnPurchaseResult().Remove(OnPurchaseResult);
		}

		CaptureInput(false);

		if (m_Root)
		{
			m_Root.RemoveFromHierarchy();
			delete m_Root;
		}
		FocusMenuRoot(true);
	}

	//------------------------------------------------------------------------------------------------
	void HandleButtonAction(string action)
	{
		if (action == "close")
		{
			Close();
			s_pActive = null;
			return;
		}

		if (action == "buy")
		{
			if (!m_Dealer)
			{
				SetStatus("Dealer unavailable.", true, false);
				return;
			}

			if (m_SelectedIndex < 0 || m_SelectedIndex >= m_Filtered.Count())
			{
				SetStatus("Select a vehicle first.", true, false);
				return;
			}

			m_Dealer.ClientPurchaseVehicle(m_SelectedPrefab);
			SetStatus("Processing purchase...", false, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetCategoryFilter(string category)
	{
		if (category != "All")
			category.ToUpper();
		m_CurrentCategory = category;
		RebuildVehicleGrid();
	}

	//------------------------------------------------------------------------------------------------
	void SelectVehicleByIndex(int index)
	{
		if (!m_Filtered || index < 0 || index >= m_Filtered.Count())
			return;

		m_SelectedIndex = index;
		ARGH_VehicleForSaleDto dto = m_Filtered[index];
		m_SelectedPrefab = dto.m_sPrefab;

		if (m_VehicleName)
			m_VehicleName.SetText(dto.m_sDisplayName);
		if (m_VehiclePrice)
			m_VehiclePrice.SetText(dto.GetPriceString());

		UpdateItemSelectionVisuals();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCatalogReceived(array<ref ARGH_VehicleForSaleDto> catalog)
	{
		m_Catalog = catalog;
		BuildCategoryButtons();
		SetCategoryFilter("All");
		SetStatus("Select a vehicle.", false, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPurchaseResult(bool success, string message)
	{
		SetStatus(message, !success, success);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildCategoryButtons()
	{
		if (!m_CategoryList)
			return;

		ClearCategoryButtons();

		map<string, bool> seen = new map<string, bool>();
		foreach (ARGH_VehicleForSaleDto dto : m_Catalog)
		{
			if (!dto)
				continue;

			string category = dto.m_sCategory;
			if (category.IsEmpty())
				category = "Other";
			category.ToUpper();

			if (seen.Contains(category))
				continue;
			seen.Set(category, true);

			Widget button = GetGame().GetWorkspace().CreateWidgets("{646A162BFADA4F76}UI/ARGH_vehicle_category_button.layout", m_CategoryList);
			if (!button)
				continue;

			TextWidget label = TextWidget.Cast(button.FindAnyWidget("CategoryText"));
			if (label)
				label.SetText(category);

			BindCategoryButton(button, category);
			m_CategoryWidgets.Insert(button);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearCategoryButtons()
	{
		if (!m_CategoryWidgets)
			return;

		foreach (Widget w : m_CategoryWidgets)
		{
			if (w)
				w.RemoveFromHierarchy();
		}

		m_CategoryWidgets.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildVehicleGrid()
	{
		if (!m_VehicleGrid)
			return;

		ClearVehicleItems();
		m_Filtered.Clear();
		m_SelectedIndex = -1;
		m_SelectedPrefab = string.Empty;

		if (m_VehicleName)
			m_VehicleName.SetText("Select a vehicle");
		if (m_VehiclePrice)
			m_VehiclePrice.SetText("$0");

		foreach (ARGH_VehicleForSaleDto dto : m_Catalog)
		{
			if (!dto)
				continue;

			if (m_CurrentCategory != "All")
			{
				string category = dto.m_sCategory;
				if (category.IsEmpty())
					category = "Other";
				category.ToUpper();
				if (category != m_CurrentCategory)
					continue;
			}

			m_Filtered.Insert(dto);
		}

		for (int i = 0; i < m_Filtered.Count(); i++)
		{
			ARGH_VehicleForSaleDto dto = m_Filtered[i];
			Widget item = GetGame().GetWorkspace().CreateWidgets("{7C149073651E4125}UI/ARGH_vehicle_dealer_item.layout", m_VehicleGrid);
			if (!item)
				continue;

			TextWidget name = TextWidget.Cast(item.FindAnyWidget("VehicleItemName"));
			TextWidget price = TextWidget.Cast(item.FindAnyWidget("VehicleItemPrice"));
			ImageWidget bg = ImageWidget.Cast(item.FindAnyWidget("VehicleItemBg"));

			if (name)
				name.SetText(dto.m_sDisplayName);
			if (price)
				price.SetText(dto.GetPriceString());
			if (bg)
				bg.SetColor(Color.FromInt(COLOR_ITEM_NORMAL));

			BindItemButton(item, i);

			m_ItemWidgets.Insert(item);
			m_ItemBackgrounds.Insert(bg);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearVehicleItems()
	{
		if (!m_ItemWidgets)
			return;

		foreach (Widget w : m_ItemWidgets)
		{
			if (w)
				w.RemoveFromHierarchy();
		}

		m_ItemWidgets.Clear();
		m_ItemBackgrounds.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateItemSelectionVisuals()
	{
		for (int i = 0; i < m_ItemBackgrounds.Count(); i++)
		{
			ImageWidget bg = m_ItemBackgrounds[i];
			if (!bg)
				continue;

			if (i == m_SelectedIndex)
				bg.SetColor(Color.FromInt(COLOR_ITEM_SELECTED));
			else
				bg.SetColor(Color.FromInt(COLOR_ITEM_NORMAL));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BindButton(Widget button, string action)
	{
		if (!button)
			return;

		ARGH_VehicleDealerOverlayButtonHandler handler = new ARGH_VehicleDealerOverlayButtonHandler();
		handler.Init(this, action);
		button.AddHandler(handler);
		m_Handlers.Insert(handler);
	}

	//------------------------------------------------------------------------------------------------
	protected void BindCategoryButton(Widget button, string category)
	{
		if (!button)
			return;

		ARGH_VehicleDealerOverlayCategoryHandler handler = new ARGH_VehicleDealerOverlayCategoryHandler();
		handler.Init(this, category);
		button.AddHandler(handler);
		m_Handlers.Insert(handler);
	}

	//------------------------------------------------------------------------------------------------
	protected void BindItemButton(Widget button, int index)
	{
		if (!button)
			return;

		ARGH_VehicleDealerOverlayItemHandler handler = new ARGH_VehicleDealerOverlayItemHandler();
		handler.Init(this, index);
		button.AddHandler(handler);
		m_Handlers.Insert(handler);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateBalances()
	{
		if (!m_WalletValue || !m_BankValue)
			return;

		float bankAmount = 0;
		float walletAmount = 0;

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(SCR_PlayerController.GetLocalPlayerId()));
		if (!pc)
			return;

		SCR_ResourceComponent playerResource = SCR_ResourceComponent.Cast(pc.FindComponent(SCR_ResourceComponent));
		if (playerResource)
		{
			SCR_ResourceContainer playerContainer = playerResource.GetContainer(EResourceType.CASH);
			if (playerContainer)
				bankAmount = playerContainer.GetResourceValue();
		}

		SCR_ChimeraCharacter char = SCR_ChimeraCharacter.Cast(pc.GetMainEntity());
		if (char)
		{
			SCR_ResourceComponent characterResource = SCR_ResourceComponent.Cast(char.FindComponent(SCR_ResourceComponent));
			if (characterResource)
			{
				SCR_ResourceContainer characterContainer = characterResource.GetContainer(EResourceType.CASH);
				if (characterContainer)
					walletAmount = characterContainer.GetResourceValue();
			}
		}

		m_BankValue.SetText("$" + FormatFloat(bankAmount, 0));
		m_WalletValue.SetText("$" + FormatFloat(walletAmount, 0));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string message, bool isError, bool isSuccess)
	{
		if (!m_StatusText)
			return;

		m_StatusText.SetText(message);
		if (isError)
			m_StatusText.SetColor(Color.FromInt(COLOR_STATUS_ERROR));
		else if (isSuccess)
			m_StatusText.SetColor(Color.FromInt(COLOR_STATUS_OK));
		else
			m_StatusText.SetColor(Color.FromInt(COLOR_STATUS_WARN));
	}

	protected void CaptureInput(bool capture)
	{
		if (capture)
			WidgetManager.SetCursor(0);
		else
			WidgetManager.SetCursor(12);

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(SCR_PlayerController.GetLocalPlayerId()));
		if (!pc)
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(pc.GetMainEntity());
		if (!character)
			return;

		m_PlayerController = CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!m_PlayerController)
			return;

		m_InteractionHandler = SCR_InteractionHandlerComponent.Cast(pc.FindComponent(SCR_InteractionHandlerComponent));

		if (capture)
		{
			m_WasMovementDisabled = m_PlayerController.GetDisableMovementControls();
			m_WasViewDisabled = m_PlayerController.GetDisableViewControls();
			m_WasWeaponDisabled = m_PlayerController.GetDisableWeaponControls();

			m_PlayerController.SetDisableMovementControls(true);
			m_PlayerController.SetDisableViewControls(true);
			m_PlayerController.SetDisableWeaponControls(true);
			m_PlayerController.OverrideMaxSpeed(0);
			m_AppliedSpeedOverride = true;
				m_PlayerController.SetWeaponNoFireTime(9999);
				m_AppliedNoFireOverride = true;

			if (m_InteractionHandler)
			{
				m_WasNearbyCollectionEnabled = m_InteractionHandler.GetNearbyCollectionEnabled();
				m_WasManualCollectionOverride = m_InteractionHandler.GetManualCollectionOverride();
				m_WasManualNearbyOverride = m_InteractionHandler.GetManualNearbyCollectionOverride();
				m_InteractionHandler.SetNearbyCollectionEnabled(false);
				m_InteractionHandler.SetManualCollectionOverride(true);
				m_InteractionHandler.SetManualNearbyCollectionOverride(true);
			}
		}
		else
		{
			m_PlayerController.SetDisableMovementControls(m_WasMovementDisabled);
			m_PlayerController.SetDisableViewControls(m_WasViewDisabled);
			m_PlayerController.SetDisableWeaponControls(m_WasWeaponDisabled);
				if (m_AppliedSpeedOverride)
				{
					m_PlayerController.OverrideMaxSpeed(1);
					m_AppliedSpeedOverride = false;
				}
				if (m_AppliedNoFireOverride)
				{
					m_PlayerController.SetWeaponNoFireTime(0);
					m_AppliedNoFireOverride = false;
				}
			if (m_InteractionHandler)
			{
				m_InteractionHandler.SetNearbyCollectionEnabled(m_WasNearbyCollectionEnabled);
				m_InteractionHandler.SetManualCollectionOverride(m_WasManualCollectionOverride);
				m_InteractionHandler.SetManualNearbyCollectionOverride(m_WasManualNearbyOverride);
			}
			m_PlayerController = null;
			m_InteractionHandler = null;
		}
	}

	protected void FocusMenuRoot(bool clear = false)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		if (clear)
		{
			workspace.SetFocusedWidget(null);
			return;
		}

		if (m_CloseButton)
			workspace.SetFocusedWidget(m_CloseButton, true);
		else if (m_Root)
			workspace.SetFocusedWidget(m_Root, true);
	}

	protected void RegisterInputListeners()
	{
		if (m_InputListenersActive)
			return;

		m_InputManager = GetGame().GetInputManager();
		if (!m_InputManager)
			return;

		m_InputManager.ActivateContext("MenuContext");
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnMenuBack);
#ifdef WORKBENCH
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, OnMenuBack);
#endif
		m_InputManager.AddActionListener("CharacterAction", EActionTrigger.DOWN, OnMenuBack);
		m_InputManager.AddActionListener("PerformAction", EActionTrigger.DOWN, OnMenuBack);
		m_InputListenersActive = true;
	}

	protected void UnregisterInputListeners()
	{
		if (!m_InputListenersActive || !m_InputManager)
			return;

		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnMenuBack);
#ifdef WORKBENCH
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, OnMenuBack);
#endif
		m_InputManager.RemoveActionListener("CharacterAction", EActionTrigger.DOWN, OnMenuBack);
		m_InputManager.RemoveActionListener("PerformAction", EActionTrigger.DOWN, OnMenuBack);
		m_InputListenersActive = false;
		m_InputManager = null;
	}

	protected void OnMenuBack()
	{
		Close();
		s_pActive = null;
	}

	protected void UpdateInputState()
	{
		if (!m_InputManager)
			m_InputManager = GetGame().GetInputManager();
		if (!m_InputManager)
			return;

		m_InputManager.ActivateContext("MenuContext");
		if (m_InputManager.IsUsingMouseAndKeyboard())
		{
			m_InputManager.SetLastUsedInputDevice(EInputDeviceType.MOUSE);
			WidgetManager.SetCursor(0);
		}

		m_InputManager.ResetAction("CharacterWeaponADS");
		m_InputManager.ResetAction("CharacterWeaponADSHold");
		m_InputManager.ResetAction("CharacterFire");
		m_InputManager.ResetAction("CharacterMelee");
		m_InputManager.ResetAction("CharacterThrowGrenade");
	}
}

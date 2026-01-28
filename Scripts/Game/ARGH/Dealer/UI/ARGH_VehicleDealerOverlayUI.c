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

	override bool OnFocus(Widget w, int x, int y)
	{
		if (!m_Menu)
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

	override bool OnFocus(Widget w, int x, int y)
	{
		// Hover should not change selection.
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!m_Menu)
			return false;

		m_Menu.SetHoverIndex(m_Index);
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_Menu)
			return false;

		m_Menu.SetHoverIndex(-1);
		return true;
	}
}

class ARGH_VehicleDealerOverlayScrollBarHandler : ScriptedWidgetEventHandler
{
	protected ARGH_VehicleDealerOverlayUI m_Menu;
	protected bool m_Dragging;

	void Init(ARGH_VehicleDealerOverlayUI menu)
	{
		m_Menu = menu;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (!m_Menu || button != 0)
			return false;

		m_Dragging = true;
		m_Menu.OnScrollBarDrag(y);
		return true;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button != 0)
			return false;

		m_Dragging = false;
		return true;
	}

	bool OnMouseMove(Widget w, int x, int y, int button)
	{
		if (!m_Menu || !m_Dragging)
			return false;

		m_Menu.OnScrollBarDrag(y);
		return true;
	}
}

class ARGH_VehicleDealerOverlayWheelHandler : ScriptedWidgetEventHandler
{
	protected ARGH_VehicleDealerOverlayUI m_Menu;

	void Init(ARGH_VehicleDealerOverlayUI menu)
	{
		m_Menu = menu;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_Menu || wheel == 0)
			return false;

		m_Menu.HandleMouseWheel(wheel);
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
	protected TextWidget m_HeaderWalletValue;
	protected TextWidget m_HeaderBankValue;
	protected TextWidget m_StatusText;
	protected ItemPreviewWidget m_VehiclePreview3D;
	protected ImageWidget m_VehiclePreviewImage;
	protected Widget m_CloseButton;
	protected Widget m_BuyButton;
	protected ImageWidget m_BuyBg;
	protected TextWidget m_BuyText;
	protected VerticalLayoutWidget m_CategoryList;
	protected VerticalLayoutWidget m_VehicleGrid;
	protected ScrollLayoutWidget m_VehicleGridScroll;
	protected Widget m_VehicleGridScrollBar;
	protected Widget m_VehicleGridScrollThumb;

	protected ARGH_VehicleDealerComponent m_Dealer;
	protected ref array<ref ARGH_VehicleForSaleDto> m_Catalog;
	protected ref array<ref ARGH_VehicleForSaleDto> m_Filtered;
	protected ref array<Widget> m_CategoryWidgets;
	protected ref array<Widget> m_CategoryButtons;
	protected ref array<ImageWidget> m_CategoryBackgrounds;
	protected ref array<TextWidget> m_CategoryTexts;
	protected ref array<string> m_CategoryNames;
	protected ref array<Widget> m_ItemWidgets;
	protected ref array<ImageWidget> m_ItemBackgrounds;
	protected ref array<TextWidget> m_ItemNames;
	protected ref array<TextWidget> m_ItemPrices;
	protected ref array<ItemPreviewWidget> m_ItemPreview3D;
	protected ref array<ImageWidget> m_ItemPreviewImages;
	protected ref array<Widget> m_ItemPreviewContainers;
	protected ref array<ref ScriptedWidgetEventHandler> m_Handlers;
	protected Widget m_CategoryAllButton;
	protected ItemPreviewManagerEntity m_PreviewManager;
	protected const int SCROLLBAR_HEIGHT = 520;
	protected const int SCROLLBAR_MIN_THUMB = 36;
protected const int SCROLLBAR_VISIBLE_ITEMS = 9;
	protected const float HOLD_INITIAL_DELAY = 0.35;
	protected const float HOLD_REPEAT_INTERVAL = 0.12;
	protected bool m_HoldUp;
	protected bool m_HoldDown;
	protected float m_HoldTimer;

	protected int m_SelectedIndex = -1;
	protected int m_HoverIndex = -1;
	protected int m_FirstVisibleIndex = 0;
	protected ResourceName m_SelectedPrefab;
	protected string m_CurrentCategory = "All";
	protected string m_CurrentStatus = string.Empty;

protected const int COLOR_ITEM_NORMAL = 0xCC1A1C24;
protected const int COLOR_ITEM_SELECTED = 0xCC1F6B3A;
protected const int COLOR_ITEM_HOVER = 0xCC244A72;
protected const int COLOR_ITEM_TEXT_NORMAL = 0xFFF1F2F5;
protected const int COLOR_ITEM_TEXT_SELECTED = 0xFFB9F7CC;
protected const int COLOR_ITEM_TEXT_HOVER = 0xFF8AD3FF;
	protected const int COLOR_BUY_BG_DEFAULT = 0xD92ECCC7;
	protected const int COLOR_BUY_TEXT_DEFAULT = 0xFF0F1217;
	protected const int COLOR_BUY_BG_SUCCESS = 0xFFFFFFFF;
	protected const int COLOR_BUY_TEXT_SUCCESS = 0xFF0F1217;
	protected const int COLOR_BUY_BG_FAIL = 0xF2D96A1E;
	protected const int COLOR_BUY_TEXT_FAIL = 0xFFFFFFFF;
	protected const string SFX_PURCHASE_SUCCESS = "SOUND_FE_BUTTON_SELECT";
	protected const string SFX_PURCHASE_FAIL = "SOUND_FE_BUTTON_HOVER";
	protected bool m_FlashSuccess;
	protected int m_FlashPulse;
protected const int NAME_MARQUEE_INTERVAL_MS = 100;
protected const int NAME_MARQUEE_WINDOW_CHARS = 64;
	protected const string NAME_MARQUEE_GAP = "   •   ";
	protected string m_NameMarqueeSource = string.Empty;
	protected string m_NameMarqueeScroll = string.Empty;
	protected int m_NameMarqueeIndex;
	protected bool m_NameMarqueeActive;
	protected const int COLOR_STATUS_OK = 0xFF7CC87C;
	protected const int COLOR_STATUS_WARN = 0xFFC18A4C;
	protected const int COLOR_STATUS_ERROR = 0xFFCE5A4C;
	protected const int COLOR_CATEGORY_NORMAL = 0xFF1F222B;
	protected const int COLOR_CATEGORY_SELECTED = 0xFF2C333D;
	protected const int COLOR_CATEGORY_TEXT_NORMAL = 0xFFF1F2F5;
	protected const int COLOR_CATEGORY_TEXT_SELECTED = 0xFF2ECCCA;

	//------------------------------------------------------------------------------------------------
	static bool IsOpen()
	{
		return s_pActive != null;
	}

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
		m_HeaderWalletValue = TextWidget.Cast(m_Root.FindAnyWidget("HeaderWalletValue"));
		m_HeaderBankValue = TextWidget.Cast(m_Root.FindAnyWidget("HeaderBankValue"));
		m_StatusText = TextWidget.Cast(m_Root.FindAnyWidget("StatusText"));
		m_VehiclePreview3D = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("VehiclePreview3D"));
		m_VehiclePreviewImage = ImageWidget.Cast(m_Root.FindAnyWidget("VehiclePreviewImage"));
		Widget previewSize = m_Root.FindAnyWidget("VehiclePreviewSize");
		if (previewSize)
			previewSize.SetVisible(true);
		m_CloseButton = m_Root.FindAnyWidget("CloseButton");
		m_BuyButton = m_Root.FindAnyWidget("BuyButton");
		m_BuyBg = ImageWidget.Cast(m_Root.FindAnyWidget("BuyBg"));
		m_BuyText = TextWidget.Cast(m_Root.FindAnyWidget("BuyText"));
		ResetBuyButtonColors();
		m_CategoryList = VerticalLayoutWidget.Cast(m_Root.FindAnyWidget("CategoryList"));
		m_VehicleGrid = VerticalLayoutWidget.Cast(m_Root.FindAnyWidget("VehicleGrid"));
		m_VehicleGridScroll = ScrollLayoutWidget.Cast(m_Root.FindAnyWidget("VehicleGridScroll"));
		m_VehicleGridScrollBar = m_Root.FindAnyWidget("VehicleGridScrollBar");
		m_VehicleGridScrollThumb = m_Root.FindAnyWidget("VehicleGridScrollThumb");

		m_Catalog = new array<ref ARGH_VehicleForSaleDto>();
		m_Filtered = new array<ref ARGH_VehicleForSaleDto>();
		m_CategoryWidgets = new array<Widget>();
		m_CategoryButtons = new array<Widget>();
		m_CategoryBackgrounds = new array<ImageWidget>();
		m_CategoryTexts = new array<TextWidget>();
		m_CategoryNames = new array<string>();
		m_ItemWidgets = new array<Widget>();
		m_ItemBackgrounds = new array<ImageWidget>();
		m_ItemNames = new array<TextWidget>();
		m_ItemPrices = new array<TextWidget>();
		m_ItemPreview3D = new array<ItemPreviewWidget>();
		m_ItemPreviewImages = new array<ImageWidget>();
		m_ItemPreviewContainers = new array<Widget>();
		m_Handlers = new array<ref ScriptedWidgetEventHandler>();

		BindButton(m_CloseButton, "close");
		BindButton(m_BuyButton, "buy");
		m_CategoryAllButton = m_Root.FindAnyWidget("CategoryAll");
		BindCategoryButton(m_CategoryAllButton, "All");
		BindScrollBar(m_VehicleGridScrollBar);
		BindScrollBar(m_VehicleGridScrollThumb);
		BindMouseWheel(m_VehicleGridScroll);
		BindMouseWheel(m_VehicleGrid);
		ResetCategoryVisuals();

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
		GetGame().GetCallqueue().CallLater(UpdateHoldRepeat, 50, true);
		RegisterInputListeners();
		FocusMenuRoot();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(UpdateBalances);
		GetGame().GetCallqueue().Remove(UpdateInputState);
		GetGame().GetCallqueue().Remove(UpdateHoldRepeat);
		GetGame().GetCallqueue().Remove(TickNameMarquee);
		GetGame().GetCallqueue().Remove(FlashBuyButtonPulse);
		GetGame().GetCallqueue().Remove(ResetBuyButtonColors);
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
			return;
		}

	}

	//------------------------------------------------------------------------------------------------
	void SetCategoryFilter(string category)
	{
		if (category != "All")
			category.ToUpper();
		m_CurrentCategory = category;
		RebuildVehicleGrid();
		UpdateCategorySelectionVisuals();
	}

	//------------------------------------------------------------------------------------------------
	void SelectVehicleByIndex(int index)
	{
		if (!m_Filtered || index < 0 || index >= m_Filtered.Count())
			return;

		m_SelectedIndex = index;
		ARGH_VehicleForSaleDto dto = m_Filtered[index];
		m_SelectedPrefab = dto.m_sPrefab;

		UpdateVehicleName(dto.m_sDisplayName);
		if (m_VehiclePrice)
			m_VehiclePrice.SetText(dto.GetPriceString());
		UpdateVehiclePreview(dto);

		EnsureSelectionVisible();
		UpdateItemSelectionVisuals();
		FocusVisibleItem(m_SelectedIndex);
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
		FlashBuyButton(success);
		PlayPurchaseSound(success);
	}

	protected void PlayPurchaseSound(bool success)
	{
		string sfx;
		if (success)
			sfx = SFX_PURCHASE_SUCCESS;
		else
			sfx = SFX_PURCHASE_FAIL;
		if (sfx.IsEmpty())
			return;
		SCR_UISoundEntity.SoundEvent(sfx, false);
	}

	protected void UpdateVehicleName(string name)
	{
		if (!m_VehicleName)
			return;

		if (name.IsEmpty())
		{
			StopNameMarquee();
			m_VehicleName.SetText(name);
			return;
		}

		if (name.Length() <= NAME_MARQUEE_WINDOW_CHARS)
		{
			StopNameMarquee();
			m_VehicleName.SetText(name);
			return;
		}

		m_NameMarqueeSource = name;
		m_NameMarqueeScroll = name + NAME_MARQUEE_GAP + name;
		m_NameMarqueeIndex = 0;
		m_NameMarqueeActive = true;

		m_VehicleName.SetText(GetMarqueeSlice());
		GetGame().GetCallqueue().Remove(TickNameMarquee);
		GetGame().GetCallqueue().CallLater(TickNameMarquee, NAME_MARQUEE_INTERVAL_MS, true);
	}

	protected string GetMarqueeSlice()
	{
		if (m_NameMarqueeScroll.IsEmpty())
			return m_NameMarqueeSource;

		int totalLen = m_NameMarqueeScroll.Length();
		int window = NAME_MARQUEE_WINDOW_CHARS;
		if (totalLen <= window)
			return m_NameMarqueeScroll;

		int start = m_NameMarqueeIndex % totalLen;
		int end = start + window;
		if (end <= totalLen)
			return m_NameMarqueeScroll.Substring(start, window);

		string part1 = m_NameMarqueeScroll.Substring(start, totalLen - start);
		string part2 = m_NameMarqueeScroll.Substring(0, end - totalLen);
		return part1 + part2;
	}

	protected void TickNameMarquee()
	{
		if (!m_NameMarqueeActive || !m_VehicleName)
			return;

		m_NameMarqueeIndex++;
		m_VehicleName.SetText(GetMarqueeSlice());
	}

	protected void StopNameMarquee()
	{
		m_NameMarqueeActive = false;
		m_NameMarqueeIndex = 0;
		m_NameMarqueeScroll = string.Empty;
		GetGame().GetCallqueue().Remove(TickNameMarquee);
	}

	protected void FlashBuyButton(bool success)
	{
		if (!m_BuyBg || !m_BuyText)
			return;

		m_FlashSuccess = success;
		m_FlashPulse = 0;
		GetGame().GetCallqueue().Remove(FlashBuyButtonPulse);
		GetGame().GetCallqueue().Remove(ResetBuyButtonColors);
		FlashBuyButtonPulse();
	}

	protected void FlashBuyButtonPulse()
	{
		if (!m_BuyBg || !m_BuyText)
			return;

		if (m_FlashSuccess)
			SetBuyButtonColors(COLOR_BUY_BG_SUCCESS, COLOR_BUY_TEXT_SUCCESS);
		else
			SetBuyButtonColors(COLOR_BUY_BG_FAIL, COLOR_BUY_TEXT_FAIL);

		GetGame().GetCallqueue().CallLater(ResetBuyButtonColors, 120, false);
		m_FlashPulse++;
		if (m_FlashPulse < 2)
			GetGame().GetCallqueue().CallLater(FlashBuyButtonPulse, 220, false);
	}

	protected void ResetBuyButtonColors()
	{
		SetBuyButtonColors(COLOR_BUY_BG_DEFAULT, COLOR_BUY_TEXT_DEFAULT);
	}

	protected void SetBuyButtonColors(int bgColor, int textColor)
	{
		if (m_BuyBg)
			m_BuyBg.SetColor(Color.FromInt(bgColor));
		if (m_BuyText)
			m_BuyText.SetColor(Color.FromInt(textColor));
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildCategoryButtons()
	{
		if (!m_CategoryList)
			return;

		ClearCategoryButtons();
		ResetCategoryVisuals();

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
			RegisterCategoryWidget(button, category);
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

		StopNameMarquee();
		if (m_VehicleName)
			m_VehicleName.SetText("Select a vehicle");
		if (m_VehiclePrice)
			m_VehiclePrice.SetText("$0");
		ClearVehiclePreview();
		m_FirstVisibleIndex = 0;

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

		BuildVisibleItems();

		ClearLoadingStatusIfCatalogReady();
		UpdateScrollBar();
		if (m_Filtered.Count() > 0)
			SelectVehicleByIndex(0);
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
	m_ItemNames.Clear();
	m_ItemPrices.Clear();
	m_ItemPreview3D.Clear();
		m_ItemPreviewImages.Clear();
		m_ItemPreviewContainers.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateItemSelectionVisuals()
	{
		for (int i = 0; i < m_ItemBackgrounds.Count(); i++)
		{
			ImageWidget bg = m_ItemBackgrounds[i];
			if (!bg)
				continue;

			int dataIndex = m_FirstVisibleIndex + i;
			if (dataIndex == m_SelectedIndex)
				bg.SetColor(Color.FromInt(COLOR_ITEM_SELECTED));
			else if (dataIndex == m_HoverIndex)
				bg.SetColor(Color.FromInt(COLOR_ITEM_HOVER));
			else
				bg.SetColor(Color.FromInt(COLOR_ITEM_NORMAL));

			TextWidget name = null;
			if (m_ItemNames && i < m_ItemNames.Count())
				name = m_ItemNames[i];
			TextWidget price = null;
			if (m_ItemPrices && i < m_ItemPrices.Count())
				price = m_ItemPrices[i];

			if (name)
			{
				if (dataIndex == m_SelectedIndex)
					name.SetColor(Color.FromInt(COLOR_ITEM_TEXT_SELECTED));
				else if (dataIndex == m_HoverIndex)
					name.SetColor(Color.FromInt(COLOR_ITEM_TEXT_HOVER));
				else
					name.SetColor(Color.FromInt(COLOR_ITEM_TEXT_NORMAL));
			}
			if (price)
			{
				if (dataIndex == m_SelectedIndex)
					price.SetColor(Color.FromInt(COLOR_ITEM_TEXT_SELECTED));
				else if (dataIndex == m_HoverIndex)
					price.SetColor(Color.FromInt(COLOR_ITEM_TEXT_HOVER));
				else
					price.SetColor(Color.FromInt(COLOR_ITEM_TEXT_NORMAL));
			}

			ItemPreviewWidget preview3D = null;
			if (m_ItemPreview3D && i < m_ItemPreview3D.Count())
				preview3D = m_ItemPreview3D[i];
			ImageWidget preview = null;
			if (m_ItemPreviewImages && i < m_ItemPreviewImages.Count())
				preview = m_ItemPreviewImages[i];
			Widget previewContainer = null;
			if (m_ItemPreviewContainers && i < m_ItemPreviewContainers.Count())
				previewContainer = m_ItemPreviewContainers[i];

			bool showPreview = false;
			bool showImage = false;
			if (dataIndex == m_SelectedIndex && dataIndex < m_Filtered.Count())
			{
				ARGH_VehicleForSaleDto dto = m_Filtered[dataIndex];
				if (dto)
				{
					if (preview && !dto.m_sThumbnailPath.IsEmpty())
					{
						preview.LoadImageTexture(0, dto.m_sThumbnailPath);
						showImage = true;
						showPreview = true;
					}
					else if (preview3D && TrySetItemPreview(preview3D, dto.m_sPrefab))
					{
						showPreview = true;
					}
				}
			}
			if (preview3D)
				preview3D.SetVisible(showPreview && !showImage);
			if (preview)
				preview.SetVisible(showImage);
			if (previewContainer)
				previewContainer.SetVisible(showPreview);
		}
		UpdateScrollBar();
	}

	void SetHoverIndex(int index)
	{
		if (m_HoverIndex == index)
			return;

		m_HoverIndex = index;
		UpdateItemSelectionVisuals();
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

	protected void BindScrollBar(Widget bar)
	{
		if (!bar)
			return;

		ARGH_VehicleDealerOverlayScrollBarHandler handler = new ARGH_VehicleDealerOverlayScrollBarHandler();
		handler.Init(this);
		bar.AddHandler(handler);
		m_Handlers.Insert(handler);
	}

	protected void BindMouseWheel(Widget target)
	{
		if (!target)
			return;

		ARGH_VehicleDealerOverlayWheelHandler handler = new ARGH_VehicleDealerOverlayWheelHandler();
		handler.Init(this);
		target.AddHandler(handler);
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

		string bankText = "$" + FormatFloat(bankAmount, 0);
		string walletText = "$" + FormatFloat(walletAmount, 0);
		m_BankValue.SetText(bankText);
		m_WalletValue.SetText(walletText);
		if (m_HeaderBankValue)
			m_HeaderBankValue.SetText(bankText);
		if (m_HeaderWalletValue)
			m_HeaderWalletValue.SetText(walletText);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string message, bool isError, bool isSuccess)
	{
		if (!m_StatusText)
			return;

		m_CurrentStatus = message;
		m_StatusText.SetText(message);
		if (isError)
			m_StatusText.SetColor(Color.FromInt(COLOR_STATUS_ERROR));
		else if (isSuccess)
			m_StatusText.SetColor(Color.FromInt(COLOR_STATUS_OK));
		else
			m_StatusText.SetColor(Color.FromInt(COLOR_STATUS_WARN));
	}

	protected void ResetCategoryVisuals()
	{
		if (!m_CategoryButtons || !m_CategoryBackgrounds || !m_CategoryTexts || !m_CategoryNames)
			return;

		m_CategoryButtons.Clear();
		m_CategoryBackgrounds.Clear();
		m_CategoryTexts.Clear();
		m_CategoryNames.Clear();

		if (m_CategoryAllButton)
			RegisterCategoryWidget(m_CategoryAllButton, "All");
	}

	protected void RegisterCategoryWidget(Widget button, string category)
	{
		if (!button || !m_CategoryButtons || !m_CategoryBackgrounds || !m_CategoryTexts || !m_CategoryNames)
			return;

		m_CategoryButtons.Insert(button);
		m_CategoryNames.Insert(category);

		ImageWidget bg = ImageWidget.Cast(button.FindAnyWidget("CategoryBg"));
		TextWidget text = TextWidget.Cast(button.FindAnyWidget("CategoryText"));
		m_CategoryBackgrounds.Insert(bg);
		m_CategoryTexts.Insert(text);
	}

	protected void UpdateCategorySelectionVisuals()
	{
		if (!m_CategoryNames || !m_CategoryBackgrounds || !m_CategoryTexts)
			return;

		for (int i = 0; i < m_CategoryNames.Count(); i++)
		{
			bool selected = m_CategoryNames[i] == m_CurrentCategory;

			ImageWidget bg = m_CategoryBackgrounds[i];
			if (bg)
			{
				if (selected)
					bg.SetColor(Color.FromInt(COLOR_CATEGORY_SELECTED));
				else
					bg.SetColor(Color.FromInt(COLOR_CATEGORY_NORMAL));
			}

			TextWidget text = m_CategoryTexts[i];
			if (text)
			{
				if (selected)
					text.SetColor(Color.FromInt(COLOR_CATEGORY_TEXT_SELECTED));
				else
					text.SetColor(Color.FromInt(COLOR_CATEGORY_TEXT_NORMAL));
			}
		}
	}

	protected void ClearLoadingStatusIfCatalogReady()
	{
		if (!m_StatusText || !m_Catalog || m_Catalog.Count() == 0)
			return;

		if (m_CurrentStatus == "Loading catalog..." || m_CurrentStatus.IsEmpty())
			SetStatus("Select a vehicle.", false, false);
	}

	protected ItemPreviewManagerEntity GetPreviewManager()
	{
		if (m_PreviewManager)
			return m_PreviewManager;

		World world = GetGame().GetWorld();
		if (!world)
			return null;

		array<IEntity> entities = {};
		world.GetActiveEntities(entities);
		foreach (IEntity ent : entities)
		{
			ItemPreviewManagerEntity manager = ItemPreviewManagerEntity.Cast(ent);
			if (manager)
			{
				m_PreviewManager = manager;
				break;
			}
		}

		return m_PreviewManager;
	}

	protected bool TrySetPreview3D(ResourceName prefab)
	{
		if (!m_VehiclePreview3D)
			return false;

		ItemPreviewManagerEntity manager = GetPreviewManager();
		if (!manager)
			return false;

		manager.SetPreviewItemFromPrefab(m_VehiclePreview3D, prefab, null, true);
		m_VehiclePreview3D.SetVisible(true);
		return true;
	}

	protected bool TrySetItemPreview(ItemPreviewWidget previewWidget, ResourceName prefab)
	{
		if (!previewWidget)
			return false;

		ItemPreviewManagerEntity manager = GetPreviewManager();
		if (!manager)
			return false;

		manager.SetPreviewItemFromPrefab(previewWidget, prefab, null, true);
		previewWidget.SetVisible(true);
		return true;
	}

	protected void ClearVehiclePreview()
	{
		if (m_VehiclePreview3D)
			m_VehiclePreview3D.SetVisible(false);
		if (m_VehiclePreviewImage)
			m_VehiclePreviewImage.SetVisible(false);
	}

	protected void UpdateVehiclePreview(ARGH_VehicleForSaleDto dto)
	{
		if (!dto)
		{
			ClearVehiclePreview();
			return;
		}

		bool previewSet = TrySetPreview3D(dto.m_sPrefab);
		bool useImage = !previewSet && !dto.m_sThumbnailPath.IsEmpty();

		if (m_VehiclePreview3D)
			m_VehiclePreview3D.SetVisible(previewSet);
		if (m_VehiclePreviewImage)
		{
			if (useImage)
			{
				m_VehiclePreviewImage.SetVisible(true);
				m_VehiclePreviewImage.LoadImageTexture(0, dto.m_sThumbnailPath);
			}
			else
			{
				m_VehiclePreviewImage.SetVisible(false);
			}
		}
	}

	protected void UpdateScrollBar()
	{
		if (!m_VehicleGridScrollBar || !m_VehicleGridScrollThumb)
			return;

		int total = 0;
		if (m_Filtered)
			total = m_Filtered.Count();
		if (total <= SCROLLBAR_VISIBLE_ITEMS)
		{
			m_VehicleGridScrollBar.SetVisible(false);
			return;
		}

		m_VehicleGridScrollBar.SetVisible(true);

		float ratio = (float)SCROLLBAR_VISIBLE_ITEMS / Math.Max(total, 1);
		float thumbHeight = Math.Max(SCROLLBAR_MIN_THUMB, SCROLLBAR_HEIGHT * ratio);
		float maxPos = Math.Max(0.0, SCROLLBAR_HEIGHT - thumbHeight);
		float t = 0.0;
		if (total > 1 && m_SelectedIndex >= 0)
			t = Math.Clamp((float)m_SelectedIndex / (float)(total - 1), 0.0, 1.0);

		float yPos = maxPos * t;
		FrameSlot.SetPos(m_VehicleGridScrollThumb, 0, yPos);
		FrameSlot.SetSize(m_VehicleGridScrollThumb, 4, thumbHeight);
	}

	protected float GetScrollBarThumbHeight(int total)
	{
		float ratio = (float)SCROLLBAR_VISIBLE_ITEMS / Math.Max(total, 1);
		return Math.Max(SCROLLBAR_MIN_THUMB, SCROLLBAR_HEIGHT * ratio);
	}

	void OnScrollBarDrag(int yPos)
	{
		int total = 0;
		if (m_Filtered)
			total = m_Filtered.Count();
		if (total <= 0)
			return;

		float thumbHeight = GetScrollBarThumbHeight(total);
		float maxPos = Math.Max(0.0, SCROLLBAR_HEIGHT - thumbHeight);
		float pos = Math.Clamp(yPos - (thumbHeight * 0.5), 0.0, maxPos);
		float t = 0.0;
		if (maxPos > 0.0)
			t = pos / maxPos;

		int index = Math.Round(t * (total - 1));
		index = Math.Clamp(index, 0, total - 1);
		SelectVehicleByIndex(index);
		FocusVisibleItem(index);
	}

	protected void FocusVisibleItem(int dataIndex)
	{
		if (!m_ItemWidgets)
			return;

		int localIndex = dataIndex - m_FirstVisibleIndex;
		if (localIndex < 0 || localIndex >= m_ItemWidgets.Count())
			return;

		Widget item = m_ItemWidgets[localIndex];
		if (!item)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		workspace.SetFocusedWidget(item, true);
	}

	protected int GetVisibleCount()
	{
		if (!m_Filtered)
			return 0;

		return Math.Min(SCROLLBAR_VISIBLE_ITEMS, m_Filtered.Count());
	}

	protected void BuildVisibleItems()
	{
		if (!m_VehicleGrid)
			return;

		ClearVehicleItems();

		int visibleCount = GetVisibleCount();
		for (int i = 0; i < visibleCount; i++)
		{
			int dataIndex = m_FirstVisibleIndex + i;
			if (dataIndex < 0 || dataIndex >= m_Filtered.Count())
				break;

			ARGH_VehicleForSaleDto dto = m_Filtered[dataIndex];
			Widget item = GetGame().GetWorkspace().CreateWidgets("{7C149073651E4125}UI/ARGH_vehicle_dealer_item.layout", m_VehicleGrid);
			if (!item)
				continue;

			TextWidget name = TextWidget.Cast(item.FindAnyWidget("VehicleItemName"));
			TextWidget price = TextWidget.Cast(item.FindAnyWidget("VehicleItemPrice"));
			ImageWidget bg = ImageWidget.Cast(item.FindAnyWidget("VehicleItemBg"));
			ItemPreviewWidget preview3D = ItemPreviewWidget.Cast(item.FindAnyWidget("VehicleItemPreview3D"));
			ImageWidget preview = ImageWidget.Cast(item.FindAnyWidget("VehicleItemPreviewImage"));
			Widget previewContainer = item.FindAnyWidget("VehicleItemPreviewSize");

			if (name)
			{
				string label = dto.m_sDisplayName;
				if (label.IsEmpty())
					label = dto.m_sPrefab;
				name.SetText(label);
			}
			if (price)
				price.SetText(dto.GetPriceString());
			if (bg)
				bg.SetColor(Color.FromInt(COLOR_ITEM_NORMAL));
			if (name)
				name.SetColor(Color.FromInt(COLOR_ITEM_TEXT_NORMAL));
			if (price)
				price.SetColor(Color.FromInt(COLOR_ITEM_TEXT_NORMAL));
			if (preview3D)
				preview3D.SetVisible(false);
			if (preview)
				preview.SetVisible(false);
			if (previewContainer)
				previewContainer.SetVisible(false);

			BindItemButton(item, dataIndex);

			m_ItemWidgets.Insert(item);
			m_ItemBackgrounds.Insert(bg);
			m_ItemNames.Insert(name);
			m_ItemPrices.Insert(price);
			m_ItemPreview3D.Insert(preview3D);
			m_ItemPreviewImages.Insert(preview);
			m_ItemPreviewContainers.Insert(previewContainer);
		}
	}

	protected void EnsureSelectionVisible()
	{
		int total = 0;
		if (m_Filtered)
			total = m_Filtered.Count();
		if (total <= 0)
			return;

		int visibleCount = GetVisibleCount();
		if (visibleCount <= 0)
			return;

		int maxFirst = Math.Max(0, total - visibleCount);
		int newFirst = m_FirstVisibleIndex;

		if (m_SelectedIndex < newFirst)
			newFirst = m_SelectedIndex;
		else if (m_SelectedIndex >= newFirst + visibleCount)
			newFirst = m_SelectedIndex - visibleCount + 1;

		newFirst = Math.Clamp(newFirst, 0, maxFirst);
		if (newFirst != m_FirstVisibleIndex)
		{
			m_FirstVisibleIndex = newFirst;
			BuildVisibleItems();
		}
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

		if (m_CategoryAllButton)
			workspace.SetFocusedWidget(m_CategoryAllButton, true);
		else if (m_CloseButton)
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
		m_InputManager.AddActionListener("MenuUp", EActionTrigger.DOWN, OnMenuUp);
		m_InputManager.AddActionListener("MenuUp", EActionTrigger.UP, OnMenuUpReleased);
		m_InputManager.AddActionListener("MenuDown", EActionTrigger.DOWN, OnMenuDown);
		m_InputManager.AddActionListener("MenuDown", EActionTrigger.UP, OnMenuDownReleased);
		m_InputManager.AddActionListener("MenuLeft", EActionTrigger.DOWN, OnMenuLeft);
		m_InputManager.AddActionListener("MenuRight", EActionTrigger.DOWN, OnMenuRight);
		m_InputManager.AddActionListener("MenuSelect", EActionTrigger.DOWN, OnMenuConfirm);
		m_InputManager.AddActionListener("MenuConfirm", EActionTrigger.DOWN, OnMenuConfirm);
		m_InputManager.AddActionListener("MenuAccept", EActionTrigger.DOWN, OnMenuConfirm);
		m_InputManager.AddActionListener("MenuAction", EActionTrigger.DOWN, OnMenuConfirm);
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
		m_InputManager.RemoveActionListener("MenuUp", EActionTrigger.DOWN, OnMenuUp);
		m_InputManager.RemoveActionListener("MenuUp", EActionTrigger.UP, OnMenuUpReleased);
		m_InputManager.RemoveActionListener("MenuDown", EActionTrigger.DOWN, OnMenuDown);
		m_InputManager.RemoveActionListener("MenuDown", EActionTrigger.UP, OnMenuDownReleased);
		m_InputManager.RemoveActionListener("MenuLeft", EActionTrigger.DOWN, OnMenuLeft);
		m_InputManager.RemoveActionListener("MenuRight", EActionTrigger.DOWN, OnMenuRight);
		m_InputManager.RemoveActionListener("MenuSelect", EActionTrigger.DOWN, OnMenuConfirm);
		m_InputManager.RemoveActionListener("MenuConfirm", EActionTrigger.DOWN, OnMenuConfirm);
		m_InputManager.RemoveActionListener("MenuAccept", EActionTrigger.DOWN, OnMenuConfirm);
		m_InputManager.RemoveActionListener("MenuAction", EActionTrigger.DOWN, OnMenuConfirm);
		m_InputListenersActive = false;
		m_InputManager = null;
	}

	protected void OnMenuBack()
	{
		Close();
		s_pActive = null;
	}

	protected void OnMenuUp()
	{
		StartHold(-1);
	}

	protected void OnMenuDown()
	{
		StartHold(1);
	}

	protected void OnMenuUpReleased()
	{
		StopHold(-1);
	}

	protected void OnMenuDownReleased()
	{
		StopHold(1);
	}

	protected void OnMenuLeft()
	{
		CycleCategory(-1);
	}

	protected void OnMenuRight()
	{
		CycleCategory(1);
	}

	protected void StartHold(int delta)
	{
		if (delta < 0)
		{
			m_HoldUp = true;
			m_HoldDown = false;
		}
		else
		{
			m_HoldDown = true;
			m_HoldUp = false;
		}

		m_HoldTimer = HOLD_INITIAL_DELAY;
		MoveSelection(delta);
	}

	protected void StopHold(int delta)
	{
		if (delta < 0)
			m_HoldUp = false;
		else
			m_HoldDown = false;
	}

	protected void UpdateHoldRepeat()
	{
		if (!m_HoldUp && !m_HoldDown)
			return;

		m_HoldTimer -= 0.05;
		if (m_HoldTimer > 0)
			return;

		if (m_HoldDown)
			MoveSelection(1);
		else
			MoveSelection(-1);
		m_HoldTimer = HOLD_REPEAT_INTERVAL;
	}

	protected void MoveSelection(int delta)
	{
		if (!m_Filtered || m_Filtered.Count() == 0)
			return;

		int index = m_SelectedIndex;
		if (index < 0)
		{
			if (delta > 0)
				index = 0;
			else
				index = m_Filtered.Count() - 1;
		}
		else
		{
			index = Math.Clamp(index + delta, 0, m_Filtered.Count() - 1);
		}

		SelectVehicleByIndex(index);
		FocusVisibleItem(index);
	}

	void HandleMouseWheel(int wheel)
	{
		int delta = 1;
		if (wheel > 0)
			delta = -1;
		MoveSelection(delta);
	}

	protected void CycleCategory(int delta)
	{
		if (!m_CategoryNames || m_CategoryNames.Count() == 0)
			return;

		int idx = m_CategoryNames.Find(m_CurrentCategory);
		if (idx < 0)
			idx = 0;

		idx += delta;
		if (idx < 0)
			idx = m_CategoryNames.Count() - 1;
		else if (idx >= m_CategoryNames.Count())
			idx = 0;

		SetCategoryFilter(m_CategoryNames[idx]);
	}

	protected void OnMenuConfirm()
	{
		if (m_SelectedIndex < 0)
			return;

		HandleButtonAction("buy");
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

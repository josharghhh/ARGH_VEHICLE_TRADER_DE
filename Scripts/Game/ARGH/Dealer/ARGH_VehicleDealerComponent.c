// ARGH_VehicleDealerComponent.c
// Shared component placed on NPC or terminal entities to provide vehicle dealership functionality.
// Handles RPC communication between client UI and server service.

[EntityEditorProps(category: "GanglandRP/Markets", description: "Vehicle dealer component for NPCs or terminals")]
class ARGH_VehicleDealerComponentClass : ScriptComponentClass
{
}

class ARGH_VehicleDealerComponent : ScriptComponent
{
	// ========================================================================
	// SECTION: ATTRIBUTES
	// ========================================================================
	
	[Attribute("", UIWidgets.EditBox, desc: "Unique dealer ID", category: "Identity")]
	protected string m_sDealerId;
	
	[Attribute("Vehicle Dealership", UIWidgets.EditBox, desc: "Dealer display name", category: "Display")]
	protected string m_sDealerName;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Dealer configuration resource", params: "conf", category: "Config")]
	protected ResourceName m_sDealerConfigPath;

	[Attribute("1", UIWidgets.CheckBox, desc: "Spawn purchased vehicle near player", category: "Spawn")]
	protected bool m_bSpawnOnPurchase;

	[Attribute("0", UIWidgets.CheckBox, desc: "Also add purchased vehicle to garage", category: "Spawn")]
	protected bool m_bAddToGarage;

	[Attribute("", UIWidgets.EditBox, desc: "Optional spawn marker entity name", category: "Spawn")]
	protected string m_sSpawnPointEntityName;

	[Attribute("2.0", UIWidgets.EditBox, desc: "Distance behind player (meters) when no marker is set", category: "Spawn")]
	protected float m_fSpawnBehindDistance;

	[Attribute("0 0 0", UIWidgets.EditBox, desc: "Local offset (right up forward) applied to spawn position", category: "Spawn")]
	protected vector m_vSpawnLocalOffset;

	[Attribute("0", UIWidgets.CheckBox, desc: "Force overlay UI (skip menu presets)", category: "UI")]
	protected bool m_bForceOverlayUI;
	
	// ========================================================================
	// SECTION: MEMBER VARIABLES
	// ========================================================================
	
	protected ref ARGH_VehicleDealerConfig m_pConfig;
	protected RplComponent m_pRplComponent;
	
	// Client-side cache of vehicle catalog
	protected ref array<ref ARGH_VehicleForSaleDto> m_aCachedCatalog;
	
	// Callback for UI when catalog received
	protected ref ScriptInvoker m_OnCatalogReceived;
	protected ref ScriptInvoker m_OnPurchaseResult;
	
	// ========================================================================
	// SECTION: INITIALIZATION
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_OnCatalogReceived = new ScriptInvoker();
		m_OnPurchaseResult = new ScriptInvoker();
		m_aCachedCatalog = new array<ref ARGH_VehicleForSaleDto>();
		
		// Load config on both client and server so UI can build catalog locally.
		LoadConfig();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void LoadConfig()
	{
		if (m_sDealerConfigPath.IsEmpty())
		{
			Print(string.Format("VehicleDealer %1: No config path set", m_sDealerId), LogLevel.WARNING);
			return;
		}
		
		Resource res = Resource.Load(m_sDealerConfigPath);
		if (!res || !res.IsValid())
		{
			Print(string.Format("VehicleDealer %1: Failed to load config: %2", m_sDealerId, m_sDealerConfigPath), LogLevel.ERROR);
			return;
		}
		
		BaseContainer container = res.GetResource().ToBaseContainer();
		if (!container)
		{
			Print(string.Format("VehicleDealer %1: Invalid config container", m_sDealerId), LogLevel.ERROR);
			return;
		}
		
		m_pConfig = ARGH_VehicleDealerConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		
		if (m_pConfig)
		{
			// Use config values if dealer ID/name not set via attributes
			if (m_sDealerId.IsEmpty() && !m_pConfig.m_sDealerId.IsEmpty())
				m_sDealerId = m_pConfig.m_sDealerId;
			if (m_sDealerName == "Vehicle Dealership" && !m_pConfig.m_sDealerName.IsEmpty())
				m_sDealerName = m_pConfig.m_sDealerName;
			
			Print(string.Format("VehicleDealer %1: Loaded %2 vehicles", m_sDealerId, m_pConfig.GetEnabledVehicleCount()));
		}
	}
	
	// ========================================================================
	// SECTION: PUBLIC API
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	string GetDealerId()
	{
		return m_sDealerId;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetDealerName()
	{
		return m_sDealerName;
	}
	
	//------------------------------------------------------------------------------------------------
	ARGH_VehicleDealerConfig GetDealerConfig()
	{
		return m_pConfig;
	}

	//------------------------------------------------------------------------------------------------
	bool ShouldSpawnOnPurchase()
	{
		return m_bSpawnOnPurchase;
	}

	//------------------------------------------------------------------------------------------------
	bool ShouldAddToGarage()
	{
		return m_bAddToGarage;
	}

	//------------------------------------------------------------------------------------------------
	bool PreferOverlayUI()
	{
		return m_bForceOverlayUI;
	}

	//------------------------------------------------------------------------------------------------
	bool TryGetSpawnTransform(int playerId, out vector transform[4])
	{
		IEntity spawnEntity = null;
		if (!m_sSpawnPointEntityName.IsEmpty())
			spawnEntity = GetGame().GetWorld().FindEntityByName(m_sSpawnPointEntityName);

		if (spawnEntity)
		{
			spawnEntity.GetTransform(transform);
			ApplyLocalOffset(transform);
			return true;
		}

		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return false;

		player.GetTransform(transform);
		transform[3] = transform[3] - (transform[2] * m_fSpawnBehindDistance);
		ApplyLocalOffset(transform);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyLocalOffset(out vector transform[4])
	{
		vector offset = m_vSpawnLocalOffset;
		if (offset == vector.Zero)
			return;

		transform[3] = transform[3] + (transform[0] * offset[0]) + (transform[1] * offset[1]) + (transform[2] * offset[2]);
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnCatalogReceived()
	{
		return m_OnCatalogReceived;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnPurchaseResult()
	{
		return m_OnPurchaseResult;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref ARGH_VehicleForSaleDto> GetCachedCatalog()
	{
		return m_aCachedCatalog;
	}
	
	// ========================================================================
	// SECTION: CLIENT-ONLY METHODS
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Client requests vehicle catalog from server
	void ClientRequestCatalog()
	{
		if (Replication.IsServer() || !m_pRplComponent)
		{
			array<ref ARGH_VehicleForSaleDto> localCatalog = BuildCatalogFromConfig();
			if (localCatalog && m_OnCatalogReceived)
				m_OnCatalogReceived.Invoke(localCatalog);
			return;
		}
		
		Rpc(RpcAsk_RequestVehicleCatalog, SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client requests to purchase a vehicle
	void ClientPurchaseVehicle(ResourceName vehiclePrefab)
	{
		if (Replication.IsServer() || !m_pRplComponent)
		{
			string errorReason;
			bool success = ProcessVehiclePurchaseLocal(SCR_PlayerController.GetLocalPlayerId(), vehiclePrefab, errorReason);
			string message;
			if (success)
				message = "Purchase successful! Vehicle added to your garage.";
			else
				message = GetErrorMessage(errorReason);
			if (m_OnPurchaseResult)
				m_OnPurchaseResult.Invoke(success, message);
			return;
		}
		
		string prefabStr = vehiclePrefab;
		Rpc(RpcAsk_PurchaseVehicle, SCR_PlayerController.GetLocalPlayerId(), prefabStr);
	}
	
	// ========================================================================
	// SECTION: RPC DEFINITIONS (CLIENT -> SERVER)
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVehicleCatalog(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		if (playerId <= 0)
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
		
		Print(string.Format("VehicleDealer %1: Catalog requested by player %2", m_sDealerId, playerId));
		
		// Get catalog from service
		ARGH_VehicleDealerServiceComponent service = ARGH_VehicleDealerServiceComponent.GetInstance();
		array<ref ARGH_VehicleForSaleDto> catalog = null;
		if (service)
		{
			catalog = service.GetVehicleCatalog(m_sDealerId);
		}
		else
		{
			Print("VehicleDealer: No VehicleDealerServiceComponent on GameMode, using local config", LogLevel.WARNING);
			catalog = BuildCatalogFromConfig();
		}

		if (!catalog)
			return;
		
		// Serialize catalog for RPC
		array<string> prefabs = {};
		array<string> names = {};
		array<int> prices = {};
		array<string> categories = {};
		array<int> seats = {};
		array<bool> licenses = {};
		array<string> thumbnails = {};
		
		foreach (ARGH_VehicleForSaleDto dto : catalog)
		{
			prefabs.Insert(dto.m_sPrefab);
			names.Insert(dto.m_sDisplayName);
			prices.Insert(dto.m_iPrice);
			categories.Insert(dto.m_sCategory);
			seats.Insert(dto.m_iSeats);
			licenses.Insert(dto.m_bRequiresLicense);
			thumbnails.Insert(dto.m_sThumbnailPath);
		}
		
		// Send to requesting player
		Rpc(RpcDo_SendVehicleCatalog, playerId, prefabs, names, prices, categories, seats, licenses, thumbnails);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_PurchaseVehicle(int playerId, string prefabStr)
	{
		if (!Replication.IsServer())
			return;
		
		if (playerId <= 0)
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
		
		Print(string.Format("VehicleDealer %1: Purchase request from player %2 for %3", m_sDealerId, playerId, prefabStr));
		
		// Process purchase via service
		ARGH_VehicleDealerServiceComponent service = ARGH_VehicleDealerServiceComponent.GetInstance();
		string errorReason;
		ResourceName prefab = prefabStr;
		bool success = false;
		if (service)
			success = service.ProcessVehiclePurchase(playerId, m_sDealerId, prefab, errorReason);
		else
			success = ProcessVehiclePurchaseLocal(playerId, prefab, errorReason);
		
		// Send result to client
		string message = "";
		if (success)
		{
			message = "Purchase successful! Vehicle added to your garage.";
		}
		else
		{
			message = GetErrorMessage(errorReason);
		}
		
		Rpc(RpcDo_PurchaseResult, playerId, success, message);
	}
	
	// ========================================================================
	// SECTION: RPC DEFINITIONS (SERVER -> CLIENT)
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SendVehicleCatalog(int playerId, array<string> prefabs, array<string> names, array<int> prices, array<string> categories, array<int> seats, array<bool> licenses, array<string> thumbnails)
	{
		if (playerId != SCR_PlayerController.GetLocalPlayerId())
			return;

		Print(string.Format("VehicleDealer: Received catalog with %1 vehicles", prefabs.Count()));
		
		// Reconstruct DTOs
		m_aCachedCatalog.Clear();
		
		for (int i = 0; i < prefabs.Count(); i++)
		{
			ARGH_VehicleForSaleDto dto = new ARGH_VehicleForSaleDto();
			dto.m_sPrefab = prefabs[i];
			dto.m_sDisplayName = names[i];
			dto.m_iPrice = prices[i];
			dto.m_sCategory = categories[i];
			dto.m_iSeats = seats[i];
			dto.m_bRequiresLicense = licenses[i];
			if (thumbnails && i < thumbnails.Count())
				dto.m_sThumbnailPath = thumbnails[i];
			dto.m_bInStock = true;
			m_aCachedCatalog.Insert(dto);
		}
		
		// Notify UI
		if (m_OnCatalogReceived)
			m_OnCatalogReceived.Invoke(m_aCachedCatalog);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PurchaseResult(int playerId, bool success, string message)
	{
		if (playerId != SCR_PlayerController.GetLocalPlayerId())
			return;

		Print(string.Format("VehicleDealer: Purchase result - success=%1, message=%2", success, message));
		
		// Notify UI
		if (m_OnPurchaseResult)
			m_OnPurchaseResult.Invoke(success, message);
	}
	
	// ========================================================================
	// SECTION: INTERNAL HELPERS
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	protected string GetErrorMessage(string errorReason)
	{
		if (errorReason == "rate_limited")
			return "Please wait before trying again.";
		if (errorReason == "invalid_player")
			return "Invalid player session.";
		if (errorReason == "dealer_not_found")
			return "Dealer not available.";
		if (errorReason == "no_dealer_config")
			return "Dealer configuration missing.";
		if (errorReason == "vehicle_not_available")
			return "This vehicle is not available.";
		if (errorReason == "invalid_price")
			return "Invalid vehicle pricing.";
		if (errorReason == "license_required")
			return "You need a driver's license to purchase this vehicle.";
		if (errorReason == "no_wallet")
			return "Wallet not found.";
		if (errorReason == "insufficient_funds")
			return "You don't have enough money.";
		if (errorReason == "economy_unavailable")
			return "Economy service unavailable.";
		if (errorReason == "spawn_failed")
			return "Failed to spawn your vehicle.";
		if (errorReason == "garage_add_failed")
			return "Failed to add vehicle to garage.";
		
		return "Purchase failed: " + errorReason;
	}

	//------------------------------------------------------------------------------------------------
	protected array<ref ARGH_VehicleForSaleDto> BuildCatalogFromConfig()
	{
		if (!m_pConfig)
		{
			Print(string.Format("VehicleDealer %1: Config missing", m_sDealerId), LogLevel.ERROR);
			return null;
		}

		array<ref ARGH_VehicleForSaleDto> result = new array<ref ARGH_VehicleForSaleDto>();
		array<ref ARGH_VehicleForSale> vehicles = m_pConfig.GetEnabledVehicles();
		foreach (ARGH_VehicleForSale v : vehicles)
		{
			if (!v)
				continue;

			int price = m_pConfig.GetVehiclePrice(v.m_sPrefab);
			ARGH_VehicleForSaleDto dto = ARGH_VehicleForSaleDto.Create(
				v.m_sPrefab,
				v.GetDisplayName(),
				price,
				v.m_sCategory,
				v.m_iSeats,
				v.m_bRequiresLicense,
				v.m_sThumbnailPath
			);
			result.Insert(dto);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ProcessVehiclePurchaseLocal(int playerId, ResourceName vehiclePrefab, out string errorReason)
	{
		errorReason = string.Empty;

		if (!m_pConfig)
		{
			errorReason = "no_dealer_config";
			return false;
		}

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
		{
			errorReason = "invalid_player";
			return false;
		}

		ARGH_VehicleForSale vehicle = m_pConfig.FindVehicleByPrefab(vehiclePrefab);
		if (!vehicle || !vehicle.IsEnabled())
		{
			errorReason = "vehicle_not_available";
			return false;
		}

		int price = m_pConfig.GetVehiclePrice(vehiclePrefab);
		if (price <= 0)
		{
			errorReason = "invalid_price";
			return false;
		}

		if (vehicle.m_bRequiresLicense)
		{
			if (!PlayerHasDriverLicense(playerId))
			{
				errorReason = "license_required";
				return false;
			}
		}

		if (!TryTakePayment(playerController, price, errorReason))
			return false;

		bool shouldSpawn = ShouldSpawnOnPurchase();
		bool shouldAddToGarage = ShouldAddToGarage();
		if (!shouldSpawn && !shouldAddToGarage)
			shouldAddToGarage = true;

		IEntity spawnedVehicle = null;
		if (shouldSpawn)
		{
			if (!SpawnPurchasedVehicle(playerId, vehiclePrefab, spawnedVehicle))
			{
				RefundPayment(playerController, price);
				errorReason = "spawn_failed";
				return false;
			}
		}

		if (shouldAddToGarage)
		{
			if (!AddVehicleToGarage(playerId, vehiclePrefab, vehicle.GetDisplayName()))
			{
				RefundPayment(playerController, price);
				errorReason = "garage_add_failed";
				if (spawnedVehicle)
					SCR_EntityHelper.DeleteEntityAndChildren(spawnedVehicle);
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool PlayerHasDriverLicense(int playerId)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool AddVehicleToGarage(int playerId, ResourceName vehiclePrefab, string displayName)
	{
		Print("[VehicleDealer] Garage integration not configured; skipping add.", LogLevel.WARNING);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SpawnPurchasedVehicle(int playerId, ResourceName vehiclePrefab, out IEntity vehicle)
	{
		vehicle = null;

		vector transform[4];
		if (!TryGetSpawnTransform(playerId, transform))
			return false;

		Resource res = Resource.Load(vehiclePrefab);
		if (!res || !res.IsValid())
			return false;

		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform = transform;
		vehicle = GetGame().SpawnEntityPrefabEx(vehiclePrefab, true, null, params);
		return vehicle != null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool TryGetWalletData(SCR_ChimeraCharacter character, out SCR_ResourceContainer wallet, out SCR_ResourceConsumer walletConsumer)
	{
		wallet = null;
		walletConsumer = null;

		if (!character)
			return false;

		SCR_ResourceComponent rc = SCR_ResourceComponent.Cast(character.FindComponent(SCR_ResourceComponent));
		if (!rc)
			return false;

		wallet = rc.GetContainer(EResourceType.CASH);
		walletConsumer = rc.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.CASH);
		return wallet != null || walletConsumer != null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool TryGetBankData(SCR_PlayerController pc, out SCR_ResourceContainer bank, out SCR_ResourceConsumer bankConsumer)
	{
		bank = null;
		bankConsumer = null;

		if (!pc)
			return false;

		SCR_ResourceComponent rc = SCR_ResourceComponent.Cast(pc.FindComponent(SCR_ResourceComponent));
		if (!rc)
			return false;

		bank = rc.GetContainer(EResourceType.CASH);
		bankConsumer = rc.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.CASH);
		return bank != null || bankConsumer != null;
	}

	//------------------------------------------------------------------------------------------------
	protected float GetFunds(SCR_ResourceContainer c, SCR_ResourceConsumer consumer)
	{
		if (c)
			return c.GetResourceValue();
		if (consumer)
			return consumer.GetAggregatedResourceValue();
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ConsumeFunds(SCR_PlayerController pc, IEntity owner, SCR_ResourceContainer c, SCR_ResourceConsumer consumer, float amount)
	{
		if (amount <= 0)
			return true;

		SCR_ResourcePlayerControllerInventoryComponent invComp =
			SCR_ResourcePlayerControllerInventoryComponent.Cast(pc.FindComponent(SCR_ResourcePlayerControllerInventoryComponent));

		if (invComp && consumer && invComp.TryPerformResourceConsumption(consumer, amount))
		{
			if (owner)
				pc.NotifyBankDataChange(Replication.FindId(owner), GetFunds(c, consumer));
			pc.NotifyPlayerDataChange(-amount);
			return true;
		}

		if (c)
		{
			c.DecreaseResourceValue(amount);
			if (owner)
				pc.NotifyBankDataChange(Replication.FindId(owner), c.GetResourceValue());
			pc.NotifyPlayerDataChange(-amount);
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool AddFunds(SCR_PlayerController pc, IEntity owner, SCR_ResourceContainer c, SCR_ResourceConsumer consumer, float amount)
	{
		if (amount <= 0 || !c)
			return false;

		c.IncreaseResourceValue(amount);
		if (owner)
			pc.NotifyBankDataChange(Replication.FindId(owner), c.GetResourceValue());
		pc.NotifyPlayerDataChange(amount);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool TryTakePayment(SCR_PlayerController pc, int price, out string errorReason)
	{
		errorReason = string.Empty;
		if (!pc)
		{
			errorReason = "invalid_player";
			return false;
		}

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(pc.GetMainEntity());
		if (!character)
		{
			errorReason = "invalid_player";
			return false;
		}

		SCR_ResourceContainer wallet;
		SCR_ResourceConsumer walletConsumer;
		bool hasWallet = TryGetWalletData(character, wallet, walletConsumer);
		float walletFunds = GetFunds(wallet, walletConsumer);

		SCR_ResourceContainer bank;
		SCR_ResourceConsumer bankConsumer;
		bool hasBank = TryGetBankData(pc, bank, bankConsumer);
		float bankFunds = GetFunds(bank, bankConsumer);

		if (!hasWallet && !hasBank)
		{
			errorReason = "no_wallet";
			return false;
		}

		float totalFunds = walletFunds + bankFunds;
		if (totalFunds < price)
		{
			errorReason = "insufficient_funds";
			return false;
		}

		float remaining = price;
		if (walletFunds > 0)
		{
			float takeWallet = Math.Min(walletFunds, remaining);
			if (!ConsumeFunds(pc, character, wallet, walletConsumer, takeWallet))
			{
				errorReason = "payment_failed";
				return false;
			}
			remaining -= takeWallet;
		}

		if (remaining > 0)
		{
			if (!ConsumeFunds(pc, pc, bank, bankConsumer, remaining))
			{
				errorReason = "payment_failed";
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefundPayment(SCR_PlayerController pc, int amount)
	{
		if (!pc || amount <= 0)
			return;

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(pc.GetMainEntity());
		if (!character)
			return;

		SCR_ResourceContainer wallet;
		SCR_ResourceConsumer walletConsumer;
		bool hasWallet = TryGetWalletData(character, wallet, walletConsumer);
		if (hasWallet && wallet)
		{
			AddFunds(pc, character, wallet, walletConsumer, amount);
			return;
		}

		SCR_ResourceContainer bank;
		SCR_ResourceConsumer bankConsumer;
		if (TryGetBankData(pc, bank, bankConsumer) && bank)
			AddFunds(pc, pc, bank, bankConsumer, amount);
	}
}

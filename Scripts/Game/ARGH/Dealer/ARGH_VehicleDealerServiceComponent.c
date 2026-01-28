// ARGH_VehicleDealerServiceComponent.c
// Server-authoritative GameMode component for vehicle market transactions.
// Handles purchase validation, payment processing, and garage storage.

class ARGH_VehicleDealerServiceComponentClass : SCR_BaseGameModeComponentClass
{
}

class ARGH_VehicleDealerServiceComponent : SCR_BaseGameModeComponent
{
	// ========================================================================
	// SECTION: SINGLETON & CONSTANTS
	// ========================================================================
	
	protected static ARGH_VehicleDealerServiceComponent s_pInstance;
	
	// Rate limiting
	static const int PURCHASE_OP_MAX_DEFAULT = 3;
	static const int PURCHASE_OP_WINDOW_SEC_DEFAULT = 10;
	
	// ========================================================================
	// SECTION: MEMBER VARIABLES
	// ========================================================================
	
	protected ref ARGH_RateLimiter m_pRateLimiter;
	protected RplComponent m_RplComponent;

	[Attribute(defvalue: "3", uiwidget: UIWidgets.EditBox, desc: "Max purchases per time window", category: "Rate Limiting")]
	protected int m_iPurchaseOpMax;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Rate limit window (seconds)", category: "Rate Limiting")]
	protected int m_iPurchaseOpWindowSec;
	
	// ========================================================================
	// SECTION: SINGLETON & INITIALIZATION
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	static ARGH_VehicleDealerServiceComponent GetInstance()
	{
		return s_pInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		s_pInstance = this;
		
		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		ARGH_VehicleDealerMenuContext.Reset();
		
		if (!Replication.IsServer())
			return;
		
		m_pRateLimiter = new ARGH_RateLimiter();
		
		Print("[VehicleDealerService] Initialized");
	}
	
	// ========================================================================
	// SECTION: PUBLIC API
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Process a vehicle purchase request
	//! \param playerId The player attempting to purchase
	//! \param dealerId The dealer's unique ID
	//! \param vehiclePrefab The vehicle prefab being purchased
	//! \param errorReason Output parameter for failure reason
	//! \return True if purchase succeeded
	bool ProcessVehiclePurchase(int playerId, string dealerId, ResourceName vehiclePrefab, out string errorReason)
	{
		if (!Replication.IsServer())
			return false;
		
		errorReason = string.Empty;
		
		// Rate limit check
		string rlKey = string.Format("vehicle_purchase_%1", playerId);
		int retryAfter;
		int maxOps = m_iPurchaseOpMax;
		if (maxOps <= 0)
			maxOps = PURCHASE_OP_MAX_DEFAULT;
		int windowSec = m_iPurchaseOpWindowSec;
		if (windowSec <= 0)
			windowSec = PURCHASE_OP_WINDOW_SEC_DEFAULT;
		if (!m_pRateLimiter.TryConsume(rlKey, maxOps, windowSec, retryAfter))
		{
			errorReason = "rate_limited";
			Print(string.Format("VehicleDealer: Rate limited player %1", playerId), LogLevel.WARNING);
			return false;
		}
		
		// Validate player
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
		{
			errorReason = "invalid_player";
			return false;
		}
		
		// Find dealer component
		ARGH_VehicleDealerComponent dealerComp = FindDealerById(dealerId);
		if (!dealerComp)
		{
			errorReason = "dealer_not_found";
			return false;
		}
		
		// Get dealer config
		ARGH_VehicleDealerConfig config = dealerComp.GetDealerConfig();
		if (!config)
		{
			errorReason = "no_dealer_config";
			return false;
		}
		
		// Validate vehicle exists in catalog
		ARGH_VehicleForSale vehicle = config.FindVehicleByPrefab(vehiclePrefab);
		if (!vehicle || !vehicle.IsEnabled())
		{
			errorReason = "vehicle_not_available";
			return false;
		}
		
		// Get price
		int price = config.GetVehiclePrice(vehiclePrefab);
		if (price <= 0)
		{
			errorReason = "invalid_price";
			return false;
		}
		
		// Check license requirement
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
		
		bool shouldSpawn = dealerComp.ShouldSpawnOnPurchase();
		bool shouldAddToGarage = dealerComp.ShouldAddToGarage();
		if (!shouldSpawn && !shouldAddToGarage)
			shouldAddToGarage = true;

		IEntity spawnedVehicle = null;
		if (shouldSpawn)
		{
			if (!SpawnPurchasedVehicle(dealerComp, playerId, vehiclePrefab, spawnedVehicle))
			{
				// Refund on failure
				RefundPayment(playerController, price);
				errorReason = "spawn_failed";
				return false;
			}
		}

		if (shouldAddToGarage)
		{
			if (!AddVehicleToGarage(playerId, vehiclePrefab, vehicle.GetDisplayName()))
			{
				// Refund on failure
				RefundPayment(playerController, price);
				errorReason = "garage_add_failed";
				if (spawnedVehicle)
					SCR_EntityHelper.DeleteEntityAndChildren(spawnedVehicle);
				return false;
			}
		}
		
		Print(string.Format("[VehicleDealerService] Player %1 purchased %2 for $%3", playerId, vehicle.GetDisplayName(), price));
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get vehicle catalog for a specific dealer
	//! \param dealerId The dealer's unique ID
	//! \return Array of vehicle DTOs available at this dealer
	array<ref ARGH_VehicleForSaleDto> GetVehicleCatalog(string dealerId)
	{
		array<ref ARGH_VehicleForSaleDto> result = new array<ref ARGH_VehicleForSaleDto>();
		
		ARGH_VehicleDealerComponent dealerComp = FindDealerById(dealerId);
		if (!dealerComp)
			return result;
		
		ARGH_VehicleDealerConfig config = dealerComp.GetDealerConfig();
		if (!config)
			return result;
		
		array<ref ARGH_VehicleForSale> vehicles = config.GetEnabledVehicles();
		foreach (ARGH_VehicleForSale v : vehicles)
		{
			if (!v)
				continue;
			
			int price = config.GetVehiclePrice(v.m_sPrefab);
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
	//! Get price for a specific vehicle at any dealer
	int GetVehiclePrice(ResourceName prefab, string dealerId = "")
	{
		// If dealer specified, use that dealer's config
		if (!dealerId.IsEmpty())
		{
			ARGH_VehicleDealerComponent dealer = FindDealerById(dealerId);
			if (dealer)
			{
				ARGH_VehicleDealerConfig cfg = dealer.GetDealerConfig();
				if (cfg)
					return cfg.GetVehiclePrice(prefab);
			}
		}
		
		// Default behavior: search all dealers (first match)
		// This is a fallback; normally you'd specify a dealer
		return -1;
	}
	
	// ========================================================================
	// SECTION: INTERNAL HELPERS
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	protected ARGH_VehicleDealerComponent FindDealerById(string dealerId)
	{
		// Search for dealer components with matching ID
		// This could be optimized with a registry pattern
		array<IEntity> entities = {};
		GetGame().GetWorld().GetActiveEntities(entities);
		
		foreach (IEntity entity : entities)
		{
			if (!entity)
				continue;
			
			ARGH_VehicleDealerComponent dealer = ARGH_VehicleDealerComponent.Cast(entity.FindComponent(ARGH_VehicleDealerComponent));
			if (dealer && dealer.GetDealerId() == dealerId)
				return dealer;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool PlayerHasDriverLicense(int playerId)
	{
		// TODO: Implement license check via character data or license system
		// For now, always return true
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool AddVehicleToGarage(int playerId, ResourceName vehiclePrefab, string displayName)
	{
		if (!Replication.IsServer())
			return false;

		Print("[VehicleDealerService] Garage integration not configured; skipping add.", LogLevel.WARNING);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SpawnPurchasedVehicle(ARGH_VehicleDealerComponent dealerComp, int playerId, ResourceName vehiclePrefab, out IEntity vehicle)
	{
		vehicle = null;

		if (!dealerComp)
			return false;

		vector transform[4];
		if (!dealerComp.TryGetSpawnTransform(playerId, transform))
			return false;

		Resource res = Resource.Load(vehiclePrefab);
		if (!res || !res.IsValid())
			return false;

		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform = transform;
		vehicle = GetGame().SpawnEntityPrefabEx(vehiclePrefab, true, null, params);
		if (!vehicle)
			return false;

		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected string GenerateLicensePlate()
	{
		// Generate random license plate
		string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		string nums = "0123456789";
		string plate = "";
		
		// Format: ABC-1234
		for (int i = 0; i < 3; i++)
		{
			int idx = Math.RandomInt(0, chars.Length());
			plate = plate + chars[idx];
		}
		plate = plate + "-";
		for (int i = 0; i < 4; i++)
		{
			int idx = Math.RandomInt(0, nums.Length());
			plate = plate + nums[idx];
		}
		
		return plate;
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

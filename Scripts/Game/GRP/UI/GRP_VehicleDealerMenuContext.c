// GRP_VehicleDealerMenuContext.c
// Simple context holder for the active dealer while opening the menu.

class GRP_VehicleDealerMenuContext
{
	protected static RplId s_rDealerId = RplId.Invalid();
	protected static bool s_bMenuOpen;

	//------------------------------------------------------------------------------------------------
	static void SetActiveDealer(GRP_VehicleDealerComponent dealer)
	{
		s_rDealerId = RplId.Invalid();
		if (!dealer)
			return;

		IEntity owner = dealer.GetOwner();
		if (!owner)
			return;

		s_rDealerId = Replication.FindId(owner);
	}

	//------------------------------------------------------------------------------------------------
	static GRP_VehicleDealerComponent GetActiveDealer()
	{
		if (!s_rDealerId.IsValid())
			return null;

		Managed item = Replication.FindItem(s_rDealerId);
		IEntity owner = IEntity.Cast(item);

		if (!owner)
			return null;

		return GRP_VehicleDealerComponent.Cast(owner.FindComponent(GRP_VehicleDealerComponent));
	}

	//------------------------------------------------------------------------------------------------
	static void Clear()
	{
		s_rDealerId = RplId.Invalid();
	}

	//------------------------------------------------------------------------------------------------
	static void SetMenuOpen(bool isOpen)
	{
		s_bMenuOpen = isOpen;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsMenuOpen()
	{
		return s_bMenuOpen;
	}
}

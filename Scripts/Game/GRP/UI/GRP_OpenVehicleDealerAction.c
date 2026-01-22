// GRP_OpenVehicleDealerAction.c
// User action to open the vehicle dealer menu.

class GRP_OpenVehicleDealerAction : SCR_ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity)
			return;

		GRP_VehicleDealerComponent dealer = GRP_VehicleDealerComponent.Cast(pOwnerEntity.FindComponent(GRP_VehicleDealerComponent));
		if (!dealer)
			return;

		if (dealer.PreferOverlayUI())
		{
			GRP_VehicleDealerMenuContext.Clear();
			GRP_VehicleDealerOverlayUI.Toggle(dealer);
			return;
		}

		GRP_VehicleDealerMenuContext.SetActiveDealer(dealer);
		MenuBase menu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GRP_VehicleDealerMenuUI);
		if (!menu)
		{
			GRP_VehicleDealerMenuContext.Clear();
			GRP_VehicleDealerOverlayUI.Toggle(dealer);
		}
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (GRP_VehicleDealerMenuContext.IsMenuOpen())
			return false;

		return super.CanBeShownScript(user);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Open Vehicle Dealer";
		return true;
	}
}

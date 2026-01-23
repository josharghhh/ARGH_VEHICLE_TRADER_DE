// ARGH_OpenVehicleDealerAction.c
// User action to open the vehicle dealer menu.

class ARGH_OpenVehicleDealerAction : SCR_ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity)
			return;

		ARGH_VehicleDealerComponent dealer = ARGH_VehicleDealerComponent.Cast(pOwnerEntity.FindComponent(ARGH_VehicleDealerComponent));
		if (!dealer)
			return;

		if (dealer.PreferOverlayUI())
		{
			ARGH_VehicleDealerMenuContext.Clear();
			ARGH_VehicleDealerOverlayUI.Toggle(dealer);
			return;
		}

		ARGH_VehicleDealerMenuContext.SetActiveDealer(dealer);
		MenuBase menu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.ARGH_VehicleDealerMenuUI);
		if (!menu)
		{
			ARGH_VehicleDealerMenuContext.Clear();
			ARGH_VehicleDealerOverlayUI.Toggle(dealer);
		}
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (ARGH_VehicleDealerMenuContext.IsMenuOpen())
			return false;

		return super.CanBeShownScript(user);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Open Vehicle Dealer";
		return true;
	}
}

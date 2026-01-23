// ARGH_VehicleForSaleDto.c
// DTO for vehicle catalog items sent from server to client.

class ARGH_VehicleForSaleDto : JsonApiStruct
{
	string m_sPrefab;
	string m_sDisplayName;
	int m_iPrice;
	string m_sCategory;
	string m_sThumbnailPath;
	int m_iSeats;
	bool m_bRequiresLicense;
	bool m_bInStock;

	//------------------------------------------------------------------------------------------------
	void ARGH_VehicleForSaleDto()
	{
		m_sPrefab = string.Empty;
		m_sDisplayName = string.Empty;
		m_iPrice = 0;
		m_sCategory = "Civilian";
		m_sThumbnailPath = string.Empty;
		m_iSeats = 4;
		m_bRequiresLicense = false;
		m_bInStock = true;

		RegV("m_sPrefab");
		RegV("m_sDisplayName");
		RegV("m_iPrice");
		RegV("m_sCategory");
		RegV("m_sThumbnailPath");
		RegV("m_iSeats");
		RegV("m_bRequiresLicense");
		RegV("m_bInStock");
	}

	//------------------------------------------------------------------------------------------------
	static ARGH_VehicleForSaleDto Create(string prefab, string displayName, int price, string category, int seats = 4, bool requiresLicense = false)
	{
		ARGH_VehicleForSaleDto dto = new ARGH_VehicleForSaleDto();
		dto.m_sPrefab = prefab;
		dto.m_sDisplayName = displayName;
		dto.m_iPrice = price;
		dto.m_sCategory = category;
		dto.m_iSeats = seats;
		dto.m_bRequiresLicense = requiresLicense;
		dto.m_bInStock = true;
		return dto;
	}

	//------------------------------------------------------------------------------------------------
	string GetPriceString()
	{
		return string.Format("$%1", m_iPrice);
	}
}

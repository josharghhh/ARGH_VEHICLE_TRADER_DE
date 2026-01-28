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
	static ARGH_VehicleForSaleDto Create(string prefab, string displayName, int price, string category, int seats = 4, bool requiresLicense = false, string thumbnailPath = "")
	{
		ARGH_VehicleForSaleDto dto = new ARGH_VehicleForSaleDto();
		dto.m_sPrefab = prefab;
		dto.m_sDisplayName = displayName;
		dto.m_iPrice = price;
		dto.m_sCategory = category;
		dto.m_sThumbnailPath = thumbnailPath;
		dto.m_iSeats = seats;
		dto.m_bRequiresLicense = requiresLicense;
		dto.m_bInStock = true;
		return dto;
	}

	//------------------------------------------------------------------------------------------------
	string GetPriceString()
	{
		return FormatMoneyInt(m_iPrice);
	}

	//------------------------------------------------------------------------------------------------
	static string FormatMoneyInt(int value)
	{
		bool negative = false;
		int absVal = value;
		if (absVal < 0)
		{
			negative = true;
			absVal = -absVal;
		}

		string raw = string.Format("%1", absVal);
		string result = "";
		int count = 0;
		for (int i = raw.Length() - 1; i >= 0; i--)
		{
			result = raw.Substring(i, 1) + result;
			count++;
			if (count == 3 && i > 0)
			{
				result = "," + result;
				count = 0;
			}
		}

		if (negative)
			result = "-" + result;

		return "$" + result;
	}

	//------------------------------------------------------------------------------------------------
	static string FormatMoneyFloat(float value)
	{
		string raw = FormatFloat(value, 0);
		bool negative = false;
		if (raw.Length() > 0 && raw.Substring(0, 1) == "-")
		{
			negative = true;
			raw = raw.Substring(1, raw.Length() - 1);
		}

		string result = "";
		int count = 0;
		for (int i = raw.Length() - 1; i >= 0; i--)
		{
			result = raw.Substring(i, 1) + result;
			count++;
			if (count == 3 && i > 0)
			{
				result = "," + result;
				count = 0;
			}
		}

		if (negative)
			result = "-" + result;

		return "$" + result;
	}
}

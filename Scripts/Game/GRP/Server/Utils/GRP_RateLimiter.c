// Simple fixed-window rate limiter for server-side anti-spam.
// Use per-player/per-action keys: e.g. string.Format("%1:vehicle_purchase", playerId).

class GRP_RateLimitBucket : Managed
{
	int m_iWindowStart;
	int m_iCount;

	void GRP_RateLimitBucket()
	{
		m_iWindowStart = 0;
		m_iCount = 0;
	}
}

class GRP_RateLimiter : Managed
{
	protected ref map<string, ref GRP_RateLimitBucket> m_mBuckets;

	void GRP_RateLimiter()
	{
		m_mBuckets = new map<string, ref GRP_RateLimitBucket>();
	}

	// Returns true if allowed and consumes a slot.
	bool TryConsume(string key, int maxPerWindow, int windowSeconds, out int retryAfterSeconds)
	{
		retryAfterSeconds = 0;
		if (key.IsEmpty())
			return true;
		if (maxPerWindow <= 0)
			return false;
		if (windowSeconds <= 0)
			return true;

		int now = System.GetUnixTime();

		GRP_RateLimitBucket bucket;
		if (!m_mBuckets.Find(key, bucket) || !bucket)
		{
			bucket = new GRP_RateLimitBucket();
			bucket.m_iWindowStart = now;
			bucket.m_iCount = 0;
			m_mBuckets.Set(key, bucket);
		}

		if ((now - bucket.m_iWindowStart) >= windowSeconds)
		{
			bucket.m_iWindowStart = now;
			bucket.m_iCount = 0;
		}

		if (bucket.m_iCount >= maxPerWindow)
		{
			retryAfterSeconds = windowSeconds - (now - bucket.m_iWindowStart);
			if (retryAfterSeconds < 1)
				retryAfterSeconds = 1;
			return false;
		}

		bucket.m_iCount++;
		return true;
	}
}

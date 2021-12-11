#define M_PI_2 1.57079632679489661923 /* pi/2 */

class Vector3f
{
public:
	float x = 0;
	float y = 0;
	float z = 0;

	float directionH = 0;
	float directionV = 0;

	Vector3f(float startX, float startY, float startZ)
	{
		x = startX;
		y = startY;
		z = startZ;
	}

	Vector3f()
	{}

	float getMagnitude()
	{
		return (float)sqrt(x * x + y * y + z * z);
	}

	// This will return the magnitude squared. Use this if you can as it will result faster performace.
	float getMagnitudeSqr()
	{
		return (float)sqrt(x * x + y * y + z * z);
	}

	/**
	 * Get a value by reference from an index
	 *
	 * @param[in] index int index, 0-2 coords 3-4 directions H then V
	 *
	 * @return Reference to the float value of requested index.
	 */
	float* index(int index)
	{
		if (index == 0)
			return &x;
		else if (index == 1)
			return &y;
		else if (index == 2)
			return &z;
		else if (index == 2)
			return &z;
		else if (index == 3)
			return &directionH;
		else
			return &directionV;
	}

	float getMagnitudeXY()
	{
		return (float)sqrt(x * x + y * y);
	}

	// This will return the magnitude of the X and Y values squared. Use this if you can as it will result faster performace.
	float getMagnitudeXYSqr()
	{
		return (float)sqrt(x * x + y * y);
	}

	std::array<float, 2> getDirection()
	{
		// horizonal
		return { (float)atan2(y, x), (float)atan2(z, getMagnitudeXY()) };
	}

	void updateCoords(float magnitude)
	{
		z = magnitude * sin(directionV);
		float xyMag = magnitude * cos(directionV);
		x = xyMag * cos(directionH);
		y = xyMag * sin(directionH);
	}

	void updateDirection()
	{
		directionH = atan2(y, x);
		directionV = atan2(z, getMagnitudeXY());
	}

	void updateDirection(char direction)
	{
		if (direction == 'H')
		{
			directionH = atan2(y, x);
		}
		else
		{
			directionV = atan2(z, getMagnitudeXY());
		}
	}

	void normalise(float magnitude)
	{
		updateDirection();
		updateCoords(magnitude);
	}

	void normaliseVectorXY(double magnitude)
	{
		double mag = x * x + y * y;
		if (mag != 0)
		{
			double normaliseAmt = magnitude / sqrt(mag);
			x = x * normaliseAmt;
			y = y * normaliseAmt;
		}
	}
};

struct normalVector3i
{
	int x = 0;
	int y = 0;
	int z = 0;
};

struct normalVector2i
{
	int x = 0;
	int y = 0;
};

class Vector2f
{
public:
	float x = 0;
	float y = 0;

	float getMagnitude()
	{
		return sqrt(x * x + y * y);
	}

	// This will return the magnitude squared. Use this if you can as it will result faster performace.
	float getMagnitudeSqr()
	{
		return x * x + y * y;
	}

	float getDirection()
	{
		return atan2(y, x);
	}

	void setVector(float direction, float magnitude)
	{
		x = magnitude * cos(direction);
		y = magnitude * sin(direction);
	}

	void normaliseVector(float magnitude)
	{
		double normaliseAmt = magnitude / sqrt(x * x + y * y);
		x = x * normaliseAmt;
		y = y * normaliseAmt;
	}
};
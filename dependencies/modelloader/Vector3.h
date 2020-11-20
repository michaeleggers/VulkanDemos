#ifndef VECTOR3_H_
#define VECTOR3_H_
namespace obj
{
	/**
	* A vector with three components.
	* \author Andreas Klein
	* \date 24.04.09
	*/
	class Vector3
	{
	public:
		float x, y, z;
		Vector3()
			:x(0),y(0),z(0)
		{

		}
		Vector3(float x_,float y_,float z_)
			:x(x_),y(y_),z(z_)			 
		{

		}

		//TODO add additional methods here
	};
}
#endif
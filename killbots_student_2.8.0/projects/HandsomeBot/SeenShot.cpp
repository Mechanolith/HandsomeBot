#include "SeenShot.h"



SeenShot::SeenShot(VisibleThing _thing)
{
	thing = _thing;
	positions.push_back(thing.position);
}


SeenShot::~SeenShot()
{
}

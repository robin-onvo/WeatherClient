
#ifndef _CITIES_H
#define _CITIES_H


#include "utils/LinkedList.h"

#define CITIES_PATH "cities"

typedef struct Cities Cities;

#include "City.h"

typedef struct Cities
{
	LinkedList list;
	
} Cities;


int Cities_Init(Cities** _CitiesPtr);

void Cities_Load(Cities* _Cities);
int Cities_LoadCity(Cities* _Cities, const char* _Path, City** _CityPtr);
void Cities_AddFromStringList(Cities* _Cities, const char* _StringList);

int Cities_Create(Cities* _Cities, const char* _Name, float _Latitude, float _Longitude, City** _CityPtr);
int Cities_GetName(Cities* _Cities, const char* _Name, City** _CityPtr);
int Cities_GetIndex(Cities* _Cities, int _Index, City** _CityPtr);
void Cities_Destroy(Cities* _Cities, City** _CityPtr);

void Cities_Print(Cities* _Cities);

void Cities_Dispose(Cities** _CitiesPtr);

#endif // _CITIES_H

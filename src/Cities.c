#include "Cities.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "utils/utils.h"

const char* Cities_list = 	"Stockholm:59.3293:18.0686\n"
							"Göteborg:57.7089:11.9746\n"
							"Malmö:55.6050:13.0038\n"
							"Uppsala:59.8586:17.6389\n"
							"Västerås:59.6099:16.5448\n"
							"Örebro:59.2741:15.2066\n"
							"Linköping:58.4109:15.6216\n"
							"Helsingborg:56.0465:12.6945\n"
							"Jönköping:57.7815:14.1562\n"
							"Norrköping:58.5877:16.1924\n"
							"Lund:55.7047:13.1910\n"
							"Gävle:60.6749:17.1413\n"
							"Sundsvall:62.3908:17.3069\n"
							"Umeå:63.8258:20.2630\n"
							"Luleå:65.5848:22.1567\n"
							"Kiruna:67.8558:20.2253\n";

int Cities_Init(Cities** _CitiesPtr)
{
	if(_CitiesPtr == NULL)
		return -1;

	Cities* _Cities = (Cities*)malloc(sizeof(Cities));
	if(_Cities == NULL)
		return -2;

	LinkedList_Initialize(&_Cities->list);

	create_folder("cities");

	//Läsa alla filer som finns i cities mappen

	Cities_AddFromStringList(_Cities, Cities_list);

	
	Cities_AddFromStringList(_Cities, Cities_list);

	*(_CitiesPtr) = _Cities;
	return 0;
}

void Cities_AddFromStringList(Cities* _Cities, const char* _StringList)
{
	char* list_copy = strdup(_StringList);
	if(list_copy == NULL)
	{
		printf("Failed to allocate memory for list copy\n");
		return;
	}

	char* ptr = list_copy;

	char* name = NULL;
	char* lat_str = NULL;
	char* lon_str = NULL;
	do
	{
		if(name == NULL)
		{
			name = ptr;
		}
		else if(lat_str == NULL)
		{
			if(*(ptr) == ':')
			{
				lat_str = ptr + 1;
				*(ptr) = '\0';
			}
		}
		else if(lon_str == NULL)
		{
			if(*(ptr) == ':')
			{
				lon_str = ptr + 1;
				*(ptr) = '\0';
			}
		}
		else
		{
			if(*(ptr) == '\n')
			{
				*(ptr) = '\0';

				//printf("City: <%s>, Latitude: <%s>, Longitude: <%s>\n", name, lat_str, lon_str);
				
				Cities_Create(_Cities, name, lat_str, lon_str, NULL);

				name = NULL;
				lat_str = NULL;
				lon_str = NULL;
			}
		}

		ptr++;

	} while (*(ptr) != '\0');

	free(list_copy);
}

int Cities_Create(Cities* _Cities, const char* _Name, const char* _Latitude, const char* _Longitude, City** _City)
{
	if(_Cities == NULL || _Name == NULL)
		return -1;

	if(Cities_GetName(_Cities, _Name, NULL) == 0)
	{
		printf("City with name '%s' already exists!\n", _Name);

		if(_City != NULL)
			*(_City) = new_City;

		return 1;
	}

	int result = 0;
	City* new_City = NULL;

	result = City_Init(_Name, _Latitude, _Longitude, &new_City);
	if(result != 0)
	{
		printf("Failed to initialize City struct! Errorcode: %i\n", result);
		return -3;
	}
	
	LinkedList_Push(&_Cities->list, new_City);
	
	if(_City != NULL)
		*(_City) = new_City;

	return 0;
}

int Cities_GetName(Cities* _Cities, const char* _Name, City** _CityPtr)
{
	if(_Cities == NULL || _Name == NULL)
		return -1;

	City* city = NULL;
	LinkedList_ForEach(&_Cities->list, &city)
	{
		if(strcmp(city->name, _Name) == 0)
		{
			if(_CityPtr != NULL)
				*(_CityPtr) = city;

			return 0;
		}
	}
	
	return -2;
}

int Cities_GetIndex(Cities* _Cities, int _Index, City** _CityPtr)
{
	if(_Cities == NULL || _CityPtr == NULL)
		return -1;

	if(_Index < 0 || _Index >= _Cities->list.length)
		return -2;

	City* city = (City*)LinkedList_Get(&_Cities->list, _Index);
	if(city == NULL)
		return -3;

	*(_CityPtr) = city;

	return 0;
}

void Cities_Destroy(Cities* _Cities, City** _CityPtr)
{
	if(_Cities == NULL || _CityPtr == NULL || *(_CityPtr) == NULL)
		return;

	City* city = *(_CityPtr);
	LinkedList_Remove(&_Cities->list, city);
	City_Dispose(&city);

	*(_CityPtr) = NULL;
}

void Cities_Print(Cities* _Cities)
{
	if(_Cities == NULL)
		return;

	int index = 1;
	City* city = NULL;
	LinkedList_ForEach(&_Cities->list, &city)
	{
		printf("[%i] - %s\n", index++, city->name);
	}
}

void Cities_Dispose(Cities** _CitiesPtr)
{
	if(_CitiesPtr == NULL || *(_CitiesPtr) == NULL)
		return;

	Cities* _Cities = *(_CitiesPtr);

	City* city = NULL;
	LinkedList_ForEach(&_Cities->list, &city)
	{
		City_Dispose(&city);
	}

	LinkedList_Dispose(&_Cities->list);

	free(_Cities);

	*(_CitiesPtr) = NULL;
}


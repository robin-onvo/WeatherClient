#include "Cities.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "utils/utils.h"
#include "utils/md5.h"
#include "utils/tinydir.h"

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

	create_folder(CITIES_PATH);

	Cities_Load(_Cities);

	*(_CitiesPtr) = _Cities;
	return 0;
}

void Cities_Load(Cities* _Cities)
{
	tinydir_dir dir;
	tinydir_open(&dir, CITIES_PATH);

	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		if (!file.is_dir)
		{
			if(strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0)
			{
				int len = strlen(file.name);
				if(len > 5 && strcmp(&file.name[len - 5], ".json") == 0)
					Cities_LoadCity(_Cities, file.path, NULL);
				
			}
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);

	Cities_AddFromStringList(_Cities, Cities_list);
}

int Cities_LoadCity(Cities* _Cities, const char* _Path, City** _CityPtr)
{
	int result = 0;

	json_t* json = json_load_file(_Path, 0, NULL);
	if(json == NULL)
	{
		printf("Failed to load city data from %s\n", _Path);
		result = -1;
		goto cleanup;
	}

	const char* name = json_string_value(json_object_get(json, "name"));
	float lat = (float)json_real_value(json_object_get(json, "latitude"));
	float lon = (float)json_real_value(json_object_get(json, "longitude"));
	if(name == NULL || lat == 0 || lon == 0) //json_real_value returns 0 on error, we assume that no city has exactly 0 latitude or longitude
	{
		printf("Invalid city data in %s! name = %s, latitude = %.2f, longitude = %.2f\n", _Path, name == NULL ? "NULL" : name, lat, lon);

		json_decref(json);
		result = -2;
		goto cleanup;
	}

	City* city = NULL;
	result = Cities_Create(_Cities, name, lat, lon, &city);
	if(result != 0)
	{
		printf("Failed to create city from data in %s. Errorcode: %i\n", _Path, result);
		json_decref(json);
		result = -3;
		goto cleanup;
	}

	if(_CityPtr != NULL)
		*(_CityPtr) = city;

	//Fall through to cleanup

cleanup:
	if(json != NULL)
		json_decref(json);

	return -4;
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
				
				Cities_Create(_Cities, name, atof(lat_str), atof(lon_str), NULL);

				name = NULL;
				lat_str = NULL;
				lon_str = NULL;
			}
		}

		ptr++;

	} while (*(ptr) != '\0');

	free(list_copy);
}

int Cities_Create(Cities* _Cities, const char* _Name, float _Latitude, float _Longitude, City** _CityPtr)
{
	if(_Cities == NULL || _Name == NULL)
		return -1;

	City* _City = NULL;
	if(Cities_GetName(_Cities, _Name, &_City) == 0)
	{
		printf("City with name '%s' already exists!\n", _Name);

		if(_CityPtr != NULL)
			*(_CityPtr) = _City;

		return 1;
	}

	int result = 0;

	result = City_Init(_Name, _Latitude, _Longitude, &_City);
	if(result != 0)
	{
		printf("Failed to initialize City struct! Errorcode: %i\n", result);
		return -3;
	}
	
	LinkedList_Push(&_Cities->list, _City);
	
	if(_CityPtr != NULL)
		*(_CityPtr) = _City;

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


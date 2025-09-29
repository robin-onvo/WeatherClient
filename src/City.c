#include "City.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils/utils.h"
#include "utils/HTTPClient.h"
#include "utils/md5.h"

//--------------Internal functions----------------

char* City_GeneratePath(City* _City);
int City_Load(City* _City);
int City_Save(City* _City);
json_t* City_GetWeatherData(City* _City);

//------------------------------------------------

int City_Init(const char* _Name, float _Latitude, float _Longitude, City** _CityPtr)
{
	if(_Name == NULL || _CityPtr == NULL)
		return -1;

	City* _City = (City*)malloc(sizeof(City));
	if(_City == NULL)
	{
		printf("Failed to allocate memory for new City\n");
		return -2;
	}

	memset(_City, 0, sizeof(City));

	_City->name = strdup(_Name);
	if(_City->name == NULL)
	{
		printf("Failed to allocate memory for City name\n");
		free(_City);
		return -3;
	}

	_City->latitude = _Latitude;
	_City->longitude = _Longitude;
	

	_City->path = City_GeneratePath(_City);
	_City->data = NULL;

	if(City_Load(_City) != 0)
	{
		City_Save(_City);
	} else {
		printf("Loaded city data from %s\n", _City->path);
	}

	*(_CityPtr) = _City;

	return 0;
}

char* City_GeneratePath(City* _City)
{
	if(_City == NULL || _City->name == NULL)
		return NULL;

	char buffer[256];
	snprintf(buffer, sizeof(buffer),
		"%s_%.4f_%.4f",
		_City->name,
		_City->latitude,
		_City->longitude);


	const char* hash = MD5_HashToString(buffer, strlen(buffer));

	printf("Unique city name(%s): %s\n", hash, buffer);

	snprintf(buffer, sizeof(buffer), "%s/%s.json", CITIES_PATH, hash);
	return strdup(buffer);
}

int City_Load(City* _City)
{
	json_t* json = json_load_file(_City->path, 0, NULL);
	if(json == NULL)
		return -1;
	
	_City->data = json;

	return 0;
}

int City_Save(City* _City)
{
	if(_City == NULL)
		return -1;

	if(_City->data == NULL)
	{
		_City->data = json_pack("{ s:s s:f s:f }",
			"name", _City->name,
			"latitude", _City->latitude,
			"longitude", _City->longitude
		);
	}
	else
	{
		//These fields should never be changed, but just to be sure...
		json_object_set_new(_City->data, "name", json_string(_City->name));
		json_object_set_new(_City->data, "latitude", json_real(_City->latitude));
		json_object_set_new(_City->data, "longitude", json_real(_City->longitude));
	}
	
	printf("Saving city data to %s\n", _City->path);
	json_dump_file(_City->data, _City->path, JSON_INDENT(4));

	return 0;
}

int City_GetValue(City* _City, const char* _Name, float* _Value, char _Unit[16])
{
	if(_City == NULL || _Name == NULL || _Value == NULL)
		return -1;

	json_t* weather = NULL;
	json_t* cache = json_object_get(_City->data, "cache");
	if(cache != NULL)
	{
		json_t* current = json_object_get(cache, "current");
		if(current != NULL)
		{
			const char* cache_time = json_string_value(json_object_get(current, "time"));
			if(cache_time != NULL)
			{
				int interval = json_integer_value(json_object_get(current, "interval"));
				if(interval < 900)
					interval = 900; // Minimum 15 minutes cache

				time_t now = time(NULL);
				time_t cacheTime = parse_iso_utc_datetime(cache_time);
				time_t nextUpdate = cacheTime + interval;
				
				if(now < nextUpdate)
				{
					weather = json_incref(cache);
					printf("Using cached weather data for City %s\n", _City->name);
				}
			}
		}
	}

	if(weather == NULL)
		weather = City_GetWeatherData(_City);

	if(weather == NULL)
	{
		printf("Failed to get weather data for City %s (Errorcode: %i)\n", _City->name, errno);
		return -2;
	}

	json_t* current = json_object_get(weather, "current");
	if(current == NULL)
	{
		printf("No 'current' field in weather data for City %s\n", _City->name);
		json_decref(weather);
		return -3;
	}

	json_t* current_units = json_object_get(weather, "current_units");
	if(current_units == NULL)
	{
		printf("No 'current_units' field in weather data for City %s\n", _City->name);
		json_decref(weather);
		return -4;
	}

	json_t* json_value = json_object_get(current, _Name);
	if(json_value == NULL)
	{
		printf("No '%s' field in weather data for City %s\n", _Name, _City->name);
		json_decref(weather);
		return -5;
	}

	json_t* json_unit = json_object_get(current_units, _Name);
	if(json_unit == NULL)
	{
		printf("No unit for '%s' field in weather data for City %s\n", _Name, _City->name);
		json_decref(weather);
		return -6;
	}

	*(_Value) = (float)json_number_value(json_value);

	const char* unit_str = json_string_value(json_unit);
	snprintf(_Unit, 16, "%s", unit_str == NULL ? "" : unit_str);


	json_object_set_new(_City->data, "cache", json_deep_copy(weather));
	

	City_Save(_City);

	json_decref(weather);
	return 0;
}

json_t* City_GetWeatherData(City* _City)
{
	int result = 0;
	HTTPClient* client = NULL;

	result = HTTPClient_Init(&client);
	if(result != 0)
	{
		printf("Failed to initialize HTTPClient for City! Errorcode: %i\n", result);
		City_Dispose(&_City);
		return NULL;
	}

	char url[512];
	snprintf(url, sizeof(url), CITY_WEATHER_API_URL, _City->latitude, _City->longitude);

	json_t* json = HTTPClient_GET(client, url);
	if(json == NULL)
	{
		printf("HTTP GET request failed! Errorcode: %i\n", errno);
		HTTPClient_Dispose(&client);
		return NULL;
	}

	json_t* ret = json_incref(json); //This means that the caller is responsible for disposing the json object. It will not be disposed when the HTTPClient is disposed.

	HTTPClient_Dispose(&client);

	return ret;
}

void City_Dispose(City** _CityPtr)
{
	if(_CityPtr == NULL || *(_CityPtr) == NULL)
		return;

	City* _City = *(_CityPtr);

	if(_City->data != NULL)
	{
		json_decref(_City->data);
		_City->data = NULL;
	}

	if(_City->name != NULL)
		free(_City->name);

	if(_City->path != NULL)
		free(_City->path);

	free(_City);
	*(_CityPtr) = NULL;
}
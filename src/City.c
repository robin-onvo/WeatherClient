#include "City.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils/utils.h"
#include "utils/HTTPClient.h"

//--------------Internal functions----------------

int City_Save(City* _City);
json_t* City_GetWeatherData(City* _City);

//------------------------------------------------


int City_Init(const char* _Name, const char* _Latitude, const char* _Longitude, City** _CityPtr)
{
	if(_Name == NULL || _CityPtr == NULL)
		return -1;

	City* _City = (City*)malloc(sizeof(City));
	if(_City == NULL)
	{
		printf("Failed to allocate memory for new City\n");
		return -1;
	}

	memset(_City, 0, sizeof(City));

	_City->name = strdup(_Name);
	if(_City->name == NULL)
	{
		printf("Failed to allocate memory for City name\n");
		free(_City);
		return -1;
	}

	if(_Latitude != NULL)
		_City->latitude = atof(_Latitude);
	else
		_City->latitude = 0.0f;
	
	if(_Longitude != NULL)
		_City->longitude = atof(_Longitude);
	else
		_City->longitude = 0.0f;


	_City->data = NULL;
	City_Save(_City);

	*(_CityPtr) = _City;

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

	char filepath[256];
	snprintf(filepath, sizeof(filepath),
		"cities/%s_lat%.2f_lon%.2f.json",
		_City->name,
		_City->latitude,
		_City->longitude);

	printf("Saving city data to %s\n", filepath);
	json_dump_file(_City->data, filepath, JSON_INDENT(4));

	
	return 0;
}

int City_GetValue(City* _City, const char* _Name, float* _Value, char _Unit[16])
{
	if(_City == NULL || _Name == NULL || _Value == NULL)
		return -1;

	json_t* weather = City_GetWeatherData(_City);
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

	if(_City->name != NULL)
		free(_City->name);

	free(_City);
	*(_CityPtr) = NULL;
}
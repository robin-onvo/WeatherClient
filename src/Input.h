
#include "Cities.h"
#include "City.h"

typedef enum
{
	Input_Command_Error = -1,
	Input_Command_OK = 0,
	Input_Command_Exit = 1,
	Input_Command_Invalid = 2
	
} Input_Command;

static inline Input_Command Input_SelectCity(Cities* _Cities, City** _CityPtr)
{
	if(_CityPtr == NULL)
		return -1;

	printf("\n-----------------------------------------\n");
	Cities_Print(_Cities);
	printf("-----------------------------------------\n");
	
	
	char buffer[100];
	printf("Select a city: ");
	if (fgets(buffer, sizeof(buffer), stdin) != NULL)
	{			
		buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline if present
		if(strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0 || strcmp(buffer, "q") == 0)
		{
			return Input_Command_Exit;
		}
		else if(strlen(buffer) > 0)
		{
			int index = -1;
			if (sscanf(buffer, "%i", &index) != 1)
				return Input_Command_Invalid;
						
			City* city = NULL;
			int result = Cities_GetIndex(_Cities, index, &city);
			if(result != 0)
				return Input_Command_Invalid;
			
			
			*(_CityPtr) = city;
			return Input_Command_OK;
		}
	}

	return Input_Command_Error;
}

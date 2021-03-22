#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <curl/curl.h>

#include "http.h"
#include "parson.h"

struct weather {
    char* weather_state_name;
    char* wind_direction_compass;
    double min_temp;
    double max_temp;
    double wind_speed;
};

static void exit_program(const char* error) {
    fprintf(stderr, "%s\n", error);
    exit(1);
}

unsigned long int parse_location(const char* raw_string) {
    JSON_Value* root_value = json_parse_string(raw_string);
    assert(json_value_get_type(root_value) == JSONArray);
    if (json_value_get_type(root_value) != JSONArray) {
        json_value_free(root_value);
        return 0;
    }

    JSON_Array* locations = json_value_get_array(root_value);
    assert(locations);
    if (!locations) {
        json_value_free(root_value);
        return 0;
    }

    json_value_free(root_value);

    size_t count = json_array_get_count(locations);
    assert(count);
    if (!count) {
        return 0;
    }

    JSON_Object* location = json_array_get_object(locations, 0);
    assert(location);
    if (!location) {
        return 0;
    }

    const long int woeid = json_object_get_number(location, "woeid");
    assert(woeid);
    if (!woeid) {
        return 0;
    }

    return woeid;
}

struct weather* parse_weather(const char* raw_string) {
    JSON_Value *root_value = json_parse_string(raw_string);
    assert(json_value_get_type(root_value) == JSONObject);
    if (json_value_get_type(root_value) != JSONObject) {
        json_value_free(root_value);
        return NULL;
    }

    JSON_Object* weather_object = json_value_get_object(root_value);
    assert(weather_object);
    if (!weather_object) {
        json_value_free(root_value);
        return NULL;
    }

    json_value_free(root_value);

    JSON_Array* consolidated_weather = json_object_get_array(weather_object, "consolidated_weather");
    assert(consolidated_weather);
    if (!consolidated_weather) {
        return NULL;
    }

    size_t count = json_array_get_count(consolidated_weather);
    assert(count);
    if (!count) {
        return NULL;
    }

    JSON_Object* first_weather_object = json_array_get_object(consolidated_weather, 0);
    assert(first_weather_object);
    if (!first_weather_object) {
        return NULL;
    }

    const char* weather_state_name = json_object_get_string(first_weather_object, "weather_state_name");
    assert(weather_state_name);
    const char* wind_direction_compass = json_object_get_string(first_weather_object, "wind_direction_compass");
    assert(wind_direction_compass);
    double min_temp = json_object_get_number(first_weather_object, "min_temp");
    double max_temp = json_object_get_number(first_weather_object, "max_temp");
    double wind_speed = json_object_get_number(first_weather_object, "wind_speed");

    if (!weather_state_name || !wind_direction_compass) {
        return NULL;
    }

    struct weather* weather = malloc(sizeof(struct weather));
    assert(weather);
    if (!weather) {
        return NULL;
    }

    char* weather_sn = malloc(strlen(weather_state_name) + 1);
    assert(weather_sn);
    if (!weather_sn) {
        free(weather);
        return NULL;
    }
    if (!strcpy(weather_sn, weather_state_name)) {
        free(weather);
        free(weather_sn);
        return NULL;
    }

    char* weather_wd = malloc(strlen(wind_direction_compass) + 1);
    assert(weather_wd);
    if (!weather_wd) {
        free(weather);
        return NULL;
    };
    if (!strcpy(weather_wd, wind_direction_compass)) {
        free(weather);
        free(weather_wd);
        return NULL;
    }

    weather->weather_state_name = weather_sn;
    weather->wind_direction_compass = weather_wd;
    weather->min_temp = min_temp;
    weather->max_temp = max_temp;
    weather->wind_speed = wind_speed;

    return weather;
}

void weather_free(struct weather* weather) {
    if (weather) {
        if (weather->weather_state_name) free(weather->weather_state_name);
        if (weather->wind_direction_compass) free(weather->wind_direction_compass);
        free(weather);
    }
}

void weather_print(struct weather* weather) {
    printf("weather print");
    printf(
        "Weather: %s, wind direction: %s, wind speed: %.2f, min temperature: %.2f, max temperature: %.2f\n",
        weather->weather_state_name,
        weather->wind_direction_compass,
        weather->wind_speed,
        weather->min_temp,
        weather->max_temp
    );
}

int main(int argc, char** argv) {
    if (argc != 2) {
        exit_program("Args must contain the location");
    }

    char* response = NULL;
    char* url = NULL;

    char* query_template = "https://www.metaweather.com/api/location/search/?query=%s";
    size_t query_needed = snprintf(NULL, 0, query_template, argv[1]) + 1;
    url = malloc(query_needed);
    assert(url);
    if (!url) {
        exit_program("Can not allocate memory for url");
    }
    sprintf(url, query_template, argv[1]);

    response = do_request(url);
    assert(response);
    if (!response) {
        free(url);
        exit_program("Can not get response from weather service");
    }

    free(url);

    unsigned long int woeid = parse_location(response);
    assert(woeid);
    if (!woeid) {
        free(response);
        exit_program("Can not parse the location response");
    }

    free(response);

    const char* city_template = "https://www.metaweather.com/api/location/%lu/";
    size_t city_needed = snprintf(NULL, 0, city_template, woeid) + 1;
    url = malloc(city_needed);
    assert(url);
    if (!url) {
        exit_program("Can not allocate memory for url");
    }
    sprintf(url, city_template, woeid);
    
    response = do_request(url);
    assert(response);
    if (!response) {
        free(url);
        exit_program("Can not get response from weather service");
    }

    free(url);

    struct weather* weather = parse_weather(response);
    assert(weather);
    if (!weather) {
        free(response);
        exit_program("Can not parse the weather response");
    }

    free(response);

    weather_print(weather);

    weather_free(weather);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <curl/curl.h>

#include "http.h"
#include "parson.h"

enum result {
    SUCCESS,
    ERROR_INPUT,
    ERROR_ARGS,
    ERROR_ALLOC,
    ERROR_PARSE,
    ERROR_CURL,
    ERROR_COPY,
    ERROR_NO_DATA
};

struct weather {
    char* weather_state_name;
    char* wind_direction_compass;
    double min_temp;
    double max_temp;
    double wind_speed;
};

static void exit_program(enum result r) {
    switch (r) {
        case ERROR_INPUT:
            fprintf(stderr, "Input must contain the location name\n");
            break;
        case ERROR_ARGS:
            fprintf(stderr, "Bad function arguments\n");
            break;
        case ERROR_ALLOC:
            fprintf(stderr, "Can not allocate memory\n");
            break;
        case ERROR_COPY:
            fprintf(stderr, "Can not copy string\n");
            break;
        case ERROR_PARSE:
            fprintf(stderr, "Json parse error\n");
            break;
        case ERROR_CURL:
            fprintf(stderr, "Weather api connection error\n");
            break;
        case ERROR_NO_DATA:
            fprintf(stderr, "Weather api contains no data\n");
            break;
        default:
            fprintf(stderr, "Uknown error\n");
    }
    exit(1);
}

enum result parse_location(const char* raw_string, unsigned long* woeid_p) {
    assert(raw_string && woeid_p);
    if (!raw_string || !woeid_p) {
        return ERROR_ARGS;
    }

    JSON_Value* root_value = json_parse_string(raw_string);
    assert(json_value_get_type(root_value) == JSONArray);
    if (json_value_get_type(root_value) != JSONArray) {
        json_value_free(root_value);
        return ERROR_PARSE;
    }

    JSON_Array* locations = json_value_get_array(root_value);
    assert(locations);
    if (!locations) {
        json_value_free(root_value);
        return ERROR_PARSE;
    }

    json_value_free(root_value);

    size_t count = json_array_get_count(locations);
    assert(count);
    if (!count) {
        return ERROR_NO_DATA;
    }

    JSON_Object* location = json_array_get_object(locations, 0);
    assert(location);
    if (!location) {
        return ERROR_PARSE;
    }

    const long int woeid = json_object_get_number(location, "woeid");
    assert(woeid);
    if (!woeid) {
        return ERROR_PARSE;
    }

    *woeid_p = woeid;
    return SUCCESS;
}

enum result parse_weather(const char* raw_string, struct weather* weather) {
    assert(raw_string && weather);
    if (!raw_string || !weather) {
        return ERROR_ARGS;
    }

    JSON_Value *root_value = json_parse_string(raw_string);
    assert(json_value_get_type(root_value) == JSONObject);
    if (json_value_get_type(root_value) != JSONObject) {
        json_value_free(root_value);
        return ERROR_PARSE;
    }

    JSON_Object* weather_object = json_value_get_object(root_value);
    assert(weather_object);
    if (!weather_object) {
        json_value_free(root_value);
        return ERROR_PARSE;
    }

    json_value_free(root_value);

    JSON_Array* consolidated_weather = json_object_get_array(weather_object, "consolidated_weather");
    assert(consolidated_weather);
    if (!consolidated_weather) {
        return ERROR_PARSE;
    }

    size_t count = json_array_get_count(consolidated_weather);
    assert(count);
    if (!count) {
        return ERROR_NO_DATA;
    }

    JSON_Object* first_weather_object = json_array_get_object(consolidated_weather, 0);
    assert(first_weather_object);
    if (!first_weather_object) {
        return ERROR_PARSE;
    }

    const char* weather_state_name = json_object_get_string(first_weather_object, "weather_state_name");
    const char* wind_direction_compass = json_object_get_string(first_weather_object, "wind_direction_compass");
    assert(weather_state_name && wind_direction_compass);
    if (!weather_state_name || !wind_direction_compass) {
        return ERROR_PARSE;
    }

    double min_temp = json_object_get_number(first_weather_object, "min_temp");
    double max_temp = json_object_get_number(first_weather_object, "max_temp");
    double wind_speed = json_object_get_number(first_weather_object, "wind_speed");

    char* weather_sn = malloc(strlen(weather_state_name) + 1);
    char* weather_wd = malloc(strlen(wind_direction_compass) + 1);
    assert(weather_sn && weather_wd);
    if (!weather_sn || !weather_wd) {
        return ERROR_ALLOC;
    }
    if (!strcpy(weather_sn, weather_state_name) || !strcpy(weather_wd, wind_direction_compass)) {
        free(weather_sn);
        free(weather_wd);
        return ERROR_COPY;
    }

    weather->weather_state_name = weather_sn;
    weather->wind_direction_compass = weather_wd;
    weather->min_temp = min_temp;
    weather->max_temp = max_temp;
    weather->wind_speed = wind_speed;

    return SUCCESS;
}

void weather_free(struct weather* weather) {
    if (!weather) {
        return;
    }
    if (weather->weather_state_name) free(weather->weather_state_name);
    if (weather->wind_direction_compass) free(weather->wind_direction_compass);
}

void weather_print(struct weather* weather) {
    if (!weather) {
        return;
    }
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
        exit_program(ERROR_INPUT);
    }

    enum result result;
    char* response = NULL;
    char* url = NULL;

    char* query_template = "https://www.metaweather.com/api/location/search/?query=%s";
    size_t query_needed = snprintf(NULL, 0, query_template, argv[1]) + 1;
    url = malloc(query_needed);
    assert(url);
    if (!url) {
        exit_program(ERROR_ALLOC);
    }
    sprintf(url, query_template, argv[1]);

    response = do_request(url);
    assert(response);
    if (!response) {
        free(url);
        exit_program(ERROR_CURL);
    }

    free(url);

    unsigned long int woeid;
    result = parse_location(response, &woeid);
    assert(result == SUCCESS);
    if (result != SUCCESS) {
        free(response);
        exit_program(result);
    }

    free(response);

    const char* city_template = "https://www.metaweather.com/api/location/%lu/";
    size_t city_needed = snprintf(NULL, 0, city_template, woeid) + 1;
    url = malloc(city_needed);
    assert(url);
    if (!url) {
        exit_program(ERROR_ALLOC);
    }
    sprintf(url, city_template, woeid);
    
    response = do_request(url);
    assert(response);
    if (!response) {
        free(url);
        exit_program(ERROR_CURL);
    }

    free(url);

    struct weather weather;
    result = parse_weather(response, &weather);
    assert(result == SUCCESS);
    if (result != SUCCESS) {
        free(response);
        exit_program(result);
    }

    free(response);

    weather_print(&weather);

    weather_free(&weather);

    return 0;
}

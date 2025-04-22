## Library Config
---
| Library_Config_Entity |       | zdj_library_config_t  |                       |
| --------------------: | :---- | --------------------: | :-------------------- |
| entity_id             | INT   | entity_id             | int                   |
| entity_counter        | INT   | entity_counter        | int                   |
| current_lib_entity_id | INT   | current_lib_entity_id | int                   |
|                       |       | current_library       | zdj_library_model_t * |
|                       |       | all_libraries [1]     | zdj_library_model_t * |
|                       |       | library_count [1]     | int                   |

[1] - Optional. Populated by zdj_library_populate_config_libraries().

### API
#### zdj_library_config_t * zdj_library_get_config( void )
Lazy-load the Config_Entity into a config_t struct and return.

#### void zdj_library_populate_config_libraries( zdj_library_config_t * config )
Create config_t instances for each Library_Entity row in db.  Set library_count to total rows in Library_Entity table


## Library
---
| Library_Entity          |      | zdj_library_t |                          |
| ----------------------: | :--- | ------------: | :----------------------- |
| entity_id               | INT  | entity_id     | int                      |
| name                    | TEXT | name          | char *                   |
| song_links              | TEXT |               |                          |
| curation_data_links [1] | TEXT |               |                          |
| setting_links           | TEXT |               |                          |

[1] - Optional. Populated by zdj_library_populate_config_libraries().

### API

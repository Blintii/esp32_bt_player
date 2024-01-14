set(ALL_PROJECT_DIRS "." "./bluetooth" "./codec")

idf_component_register(SRC_DIRS ${ALL_PROJECT_DIRS}
                        INCLUDE_DIRS ${ALL_PROJECT_DIRS})

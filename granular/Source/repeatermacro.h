/*
  ==============================================================================

    repeatermacro.h
    Created: 22 Dec 2018 1:24:08pm
    Author:  Travis West

  ==============================================================================
*/

#pragma once

#define REPEATX1(...) __VA_ARGS__

#define REPEATX2(...) REPEATX1(__VA_ARGS__), REPEATX1(__VA_ARGS__)

#define REPEATX4(...) REPEATX2(__VA_ARGS__), REPEATX2(__VA_ARGS__)

#define REPEATX8(...) REPEATX4(__VA_ARGS__), REPEATX4(__VA_ARGS__)

#define REPEATX16(...) REPEATX8(__VA_ARGS__), REPEATX8(__VA_ARGS__)

#define REPEATX32(...) REPEATX16(__VA_ARGS__), REPEATX16(__VA_ARGS__)

#define REPEATX64(...) REPEATX32(__VA_ARGS__), REPEATX32(__VA_ARGS__)

#define REPEATX128(...) REPEATX64(__VA_ARGS__), REPEATX64(__VA_ARGS__)

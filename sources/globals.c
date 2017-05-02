#include "includes/globals.h"

static int MAX_RATIO_COLD_LIQUID = MACRO_MAX_RATIO_COLD_LIQUID;
static int MAX_RATIO_LIQUID_FORZEN = MACRO_MAX_RATIO_FROZEN_LIQUID;

/*******************************************
 * get methods
 *******************************************/

int getRatioColdLiquid(void) { return MAX_RATIO_COLD_LIQUID; }

int getRatioFrozenLiquid(void) { return MAX_RATIO_LIQUID_FORZEN; }

/*******************************************
 * set methods
 *******************************************/

void setRatioColdLiquid(int ratio) { MAX_RATIO_COLD_LIQUID = ratio; }

void setRatioFrozenLiquid(int ratio) { MAX_RATIO_LIQUID_FORZEN = ratio; }

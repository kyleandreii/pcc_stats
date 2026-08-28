/**
 * Aircon Cost Calculator Module
 * Computes electricity costs based on AC usage hours, power ratings, and electricity rates
 */

const AirconCostCalculator = (function() {
    
    // Default temperature calibration points (from measured data: 16°C = 3795W, 30°C = 253W)
    const DEFAULT_TEMP_CALIBRATION = [
        { temp: 16, amps: 16.5, watts: 3795 },
        { temp: 30, amps: 1.1, watts: 253 }
    ];

    // Default power ratings (watts) by HP rating for different AC types
    const POWER_RATINGS = {
        inverter: {
            0.5: 400,
            0.75: 550,
            1.0: 700,
            1.5: 1000,
            2.0: 1400,
            2.5: 1800
        },
        non_inverter: {
            0.5: 500,
            0.75: 700,
            1.0: 900,
            1.5: 1300,
            2.0: 1800,
            2.5: 2200
        }
    };

    /**
     * Get effective wattage based on target temperature using linear interpolation
     * @param {Object} unitConfig - Unit configuration { specs: { tempCalibration: [...] }, rated_watts, hp_rating, aircon_type }
     * @param {number} targetTemp - Target temperature in Celsius
     * @returns {number} Effective wattage
     */
    function getEffectiveWatts(unitConfig, targetTemp) {
        console.log('[Cost Calculator] getEffectiveWatts called:', { unitConfig, targetTemp });
        
        if (!unitConfig || targetTemp === undefined || targetTemp === null) {
            console.log('[Cost Calculator] getEffectiveWatts - missing input, falling back to rated_watts');
            return getRatedWatts(unitConfig);
        }

        // Get calibration data from specs or use default
        const calibration = unitConfig.specs?.tempCalibration || DEFAULT_TEMP_CALIBRATION;
        
        if (!calibration || calibration.length < 2) {
            console.log('[Cost Calculator] getEffectiveWatts - no calibration data, falling back to rated_watts');
            return getRatedWatts(unitConfig);
        }

        // Sort calibration by temperature
        const sortedCal = [...calibration].sort((a, b) => a.temp - b.temp);

        // Clamp to calibration range
        if (targetTemp <= sortedCal[0].temp) {
            console.log('[Cost Calculator] getEffectiveWatts - temp below calibration range, using minimum:', sortedCal[0].watts);
            return sortedCal[0].watts;
        }
        if (targetTemp >= sortedCal[sortedCal.length - 1].temp) {
            console.log('[Cost Calculator] getEffectiveWatts - temp above calibration range, using maximum:', sortedCal[sortedCal.length - 1].watts);
            return sortedCal[sortedCal.length - 1].watts;
        }

        // Find surrounding calibration points for interpolation
        let lower = null, upper = null;
        for (let i = 0; i < sortedCal.length - 1; i++) {
            if (targetTemp >= sortedCal[i].temp && targetTemp <= sortedCal[i + 1].temp) {
                lower = sortedCal[i];
                upper = sortedCal[i + 1];
                break;
            }
        }

        if (!lower || !upper) {
            console.log('[Cost Calculator] getEffectiveWatts - could not find interpolation points, falling back to rated_watts');
            return getRatedWatts(unitConfig);
        }

        // Linear interpolation
        const tempRange = upper.temp - lower.temp;
        const wattsRange = upper.watts - lower.watts;
        const tempFraction = (targetTemp - lower.temp) / tempRange;
        const effectiveWatts = lower.watts + (wattsRange * tempFraction);

        console.log('[Cost Calculator] getEffectiveWatts - interpolated:', { 
            lower: lower.watts, 
            upper: upper.watts, 
            tempFraction, 
            effectiveWatts 
        });

        return effectiveWatts;
    }

    /**
     * Get rated watts from unit config (fallback for when calibration is unavailable)
     * @param {Object} unitConfig - Unit configuration
     * @returns {number} Rated watts
     */
    function getRatedWatts(unitConfig) {
        let watts = unitConfig.rated_watts;
        if (!watts && unitConfig.hp_rating) {
            watts = POWER_RATINGS[unitConfig.aircon_type]?.[unitConfig.hp_rating] || 0;
        }
        return watts || 0;
    }

    /**
     * Compute aircon electricity cost for a given room and usage
     * @param {Object} roomConfig - Room configuration { aircon_type, hp_rating, rated_watts, duty_cycle, units: { unit_1: {...}, unit_2: {...} } }
     * @param {number} hours - Usage hours (or object with unit-specific hours for dual-unit rooms)
     * @param {number} ratePerKwh - Electricity rate per kWh
     * @param {number} targetTemp - Target temperature in Celsius (optional, for temperature-based wattage)
     * @returns {number} Cost in currency
     */
    function computeAirconCost(roomConfig, hours, ratePerKwh, targetTemp) {
        if (!roomConfig || ratePerKwh <= 0) {
            return 0;
        }

        // Check if this is a dual-unit room
        if (roomConfig.units && (roomConfig.units.unit_1 || roomConfig.units.unit_2)) {
            let totalCost = 0;
            
            // Calculate cost for each unit independently
            if (roomConfig.units.unit_1) {
                const unit1Hours = (typeof hours === 'object' && hours.unit_1 !== undefined) ? hours.unit_1 : hours;
                const unit1TargetTemp = (typeof targetTemp === 'object' && targetTemp.unit_1 !== undefined) ? targetTemp.unit_1 : targetTemp;
                totalCost += computeSingleUnitCost(roomConfig.units.unit_1, unit1Hours, ratePerKwh, unit1TargetTemp);
            }
            
            if (roomConfig.units.unit_2) {
                const unit2Hours = (typeof hours === 'object' && hours.unit_2 !== undefined) ? hours.unit_2 : hours;
                const unit2TargetTemp = (typeof targetTemp === 'object' && targetTemp.unit_2 !== undefined) ? targetTemp.unit_2 : targetTemp;
                totalCost += computeSingleUnitCost(roomConfig.units.unit_2, unit2Hours, ratePerKwh, unit2TargetTemp);
            }
            
            return parseFloat(totalCost.toFixed(2));
        }

        // Single unit calculation (legacy)
        if (hours <= 0) return 0;
        return computeSingleUnitCost(roomConfig, hours, ratePerKwh, targetTemp);
    }

    /**
     * Compute cost for a single AC unit
     * @param {Object} unitConfig - Unit configuration { aircon_type, hp_rating, rated_watts, duty_cycle }
     * @param {number} hours - Usage hours
     * @param {number} ratePerKwh - Electricity rate per kWh
     * @param {number} targetTemp - Target temperature in Celsius (optional, for temperature-based wattage)
     * @returns {number} Cost in currency
     */
    function computeSingleUnitCost(unitConfig, hours, ratePerKwh, targetTemp) {
        console.log('[Cost Calculator] computeSingleUnitCost called:', { unitConfig, hours, ratePerKwh, targetTemp });
        
        if (!unitConfig || hours <= 0 || ratePerKwh <= 0) {
            console.log('[Cost Calculator] Returning 0 - invalid input');
            return 0;
        }

        // Get effective wattage based on target temperature, or fall back to rated watts
        let watts;
        if (targetTemp !== undefined && targetTemp !== null) {
            watts = getEffectiveWatts(unitConfig, targetTemp);
        } else {
            watts = getRatedWatts(unitConfig);
        }

        if (watts <= 0) {
            console.log('[Cost Calculator] Returning 0 - invalid watts:', watts);
            return 0;
        }

        // Get duty cycle (default to 1.0 if not specified)
        const dutyCycle = unitConfig.duty_cycle || 1.0;

        // Calculate average power in kW (accounting for duty cycle)
        const avgPowerKw = (watts * dutyCycle) / 1000;

        // Calculate energy in kWh
        const energyKwh = avgPowerKw * hours;

        // Calculate cost
        const cost = energyKwh * ratePerKwh;

        console.log('[Cost Calculator] Calculation result:', { watts, dutyCycle, avgPowerKw, energyKwh, cost });

        return parseFloat(cost.toFixed(2));
    }

    /**
     * Compute daily totals for all rooms
     * @param {Object} roomsConfig - Configuration for all rooms { roomKey: { aircon_type, hp_rating, rated_watts } }
     * @param {Object} dailyUsage - Daily usage data { roomKey: { dateStr: hours } }
     * @param {Array} dates - Array of date strings to compute
     * @param {number} ratePerKwh - Electricity rate per kWh
     * @returns {Object} Computed costs { roomKey: { dateStr: cost }, dailyTotals: { dateStr: cost }, weeklyTotal: cost }
     */
    function computeDailyTotals(roomsConfig, dailyUsage, dates, ratePerKwh) {
        const results = {
            roomCosts: {},
            dailyTotals: {},
            weeklyTotal: 0,
            roomWeeklyTotals: {}
        };

        // Initialize room costs and weekly totals
        Object.keys(roomsConfig).forEach(room => {
            results.roomCosts[room] = {};
            results.roomWeeklyTotals[room] = { hours: 0, cost: 0 };
        });

        dates.forEach(dateStr => {
            let dailyTotal = 0;
            
            Object.keys(roomsConfig).forEach(room => {
                const hours = dailyUsage[room]?.[dateStr] || 0;
                const cost = computeAirconCost(roomsConfig[room], hours, ratePerKwh);
                
                results.roomCosts[room][dateStr] = cost;
                dailyTotal += cost;
                
                results.roomWeeklyTotals[room].hours += hours;
                results.roomWeeklyTotals[room].cost += cost;
            });
            
            results.dailyTotals[dateStr] = parseFloat(dailyTotal.toFixed(2));
            results.weeklyTotal += dailyTotal;
        });

        results.weeklyTotal = parseFloat(results.weeklyTotal.toFixed(2));

        return results;
    }

    /**
     * Get default room configuration template
     * @returns {Object} Default configuration structure
     */
    function getDefaultRoomConfig() {
        return {
            aircon_type: 'inverter',
            hp_rating: 1.0,
            rated_watts: null
        };
    }

    // Public API
    return {
        computeAirconCost,
        computeDailyTotals,
        getDefaultRoomConfig,
        getEffectiveWatts,
        POWER_RATINGS
    };

})();

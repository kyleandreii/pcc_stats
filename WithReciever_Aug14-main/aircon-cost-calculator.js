/**
 * Aircon Cost Calculator Module
 * Computes electricity costs based on AC usage hours, power ratings, and electricity rates
 */

const AirconCostCalculator = (function() {
    
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
     * Compute aircon electricity cost for a given room and usage
     * @param {Object} roomConfig - Room configuration { aircon_type, hp_rating, rated_watts, duty_cycle, units: { unit_1: {...}, unit_2: {...} } }
     * @param {number} hours - Usage hours (or object with unit-specific hours for dual-unit rooms)
     * @param {number} ratePerKwh - Electricity rate per kWh
     * @returns {number} Cost in currency
     */
    function computeAirconCost(roomConfig, hours, ratePerKwh) {
        if (!roomConfig || ratePerKwh <= 0) {
            return 0;
        }

        // Check if this is a dual-unit room
        if (roomConfig.units && (roomConfig.units.unit_1 || roomConfig.units.unit_2)) {
            let totalCost = 0;
            
            // Calculate cost for each unit independently
            if (roomConfig.units.unit_1) {
                const unit1Hours = (typeof hours === 'object' && hours.unit_1 !== undefined) ? hours.unit_1 : hours;
                totalCost += computeSingleUnitCost(roomConfig.units.unit_1, unit1Hours, ratePerKwh);
            }
            
            if (roomConfig.units.unit_2) {
                const unit2Hours = (typeof hours === 'object' && hours.unit_2 !== undefined) ? hours.unit_2 : hours;
                totalCost += computeSingleUnitCost(roomConfig.units.unit_2, unit2Hours, ratePerKwh);
            }
            
            return parseFloat(totalCost.toFixed(2));
        }

        // Single unit calculation (legacy)
        if (hours <= 0) return 0;
        return computeSingleUnitCost(roomConfig, hours, ratePerKwh);
    }

    /**
     * Compute cost for a single AC unit
     * @param {Object} unitConfig - Unit configuration { aircon_type, hp_rating, rated_watts, duty_cycle }
     * @param {number} hours - Usage hours
     * @param {number} ratePerKwh - Electricity rate per kWh
     * @returns {number} Cost in currency
     */
    function computeSingleUnitCost(unitConfig, hours, ratePerKwh) {
        console.log('[Cost Calculator] computeSingleUnitCost called:', { unitConfig, hours, ratePerKwh });
        
        if (!unitConfig || hours <= 0 || ratePerKwh <= 0) {
            console.log('[Cost Calculator] Returning 0 - invalid input');
            return 0;
        }

        // Get power rating in watts
        let watts = unitConfig.rated_watts;
        if (!watts && unitConfig.hp_rating) {
            watts = POWER_RATINGS[unitConfig.aircon_type]?.[unitConfig.hp_rating] || 0;
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
        POWER_RATINGS
    };

})();

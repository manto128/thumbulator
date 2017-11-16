#ifndef EH_SIM_ON_DEMAND_ALL_BACKUP_HPP
#define EH_SIM_ON_DEMAND_ALL_BACKUP_HPP

#include "scheme/eh_scheme.hpp"
#include "capacitor.hpp"
#include "stats.hpp"

namespace ehsim {

/**
 * Based on Architecture Exploration for Ambient Energy Harvesting Nonvolatile Processors.
 *
 * See the data relating to the ODAB scheme.
 */
class on_demand_all_backup : public eh_scheme {
public:
  on_demand_all_backup() : battery(capacitance, 7.5)
  {
  }

  capacitor &get_battery() override
  {
    return battery;
  }

  uint32_t clock_frequency() const override
  {
    return cpu_frequency;
  }

  void execute_instruction(stats_bundle *stats) override
  {
    battery.consume_energy(normal_running_energy);

    stats->models.back().instruction_energy += normal_running_energy;
  }

  bool is_active() const override
  {
    return battery.energy_stored() >
           (normal_running_energy + backup_energy_penalty + recovery_energy_penalty);
  }

  bool will_backup(stats_bundle *stats) const override
  {
    return battery.energy_stored() > backup_energy_penalty;
  }

  uint64_t backup(stats_bundle *stats) override
  {
    // do not touch arch/app state, assume it is all non-volatile

    battery.consume_energy(backup_energy_penalty);

    stats->models.back().backup_times.push_back(stats->cpu.cycle_count - last_cycle_count);
    last_cycle_count = stats->cpu.cycle_count;

    return backup_time_penalty;
  }

  uint64_t restore(stats_bundle *stats) override
  {
    // do not touch arch/app state, assume it is all non-volatile

    battery.consume_energy(recovery_energy_penalty);

    // allocate space for a new active period model
    stats->models.emplace_back();

    return restore_time_penalty;
  }

private:
  capacitor battery;

  uint64_t last_cycle_count = 0u;

  // see Section 3 from paper - 8 KHz clock frequency
  static constexpr uint32_t cpu_frequency = 8000;
  // see Figure 11 from paper - instruction energy is 31.25 pJ
  static constexpr auto normal_running_energy = 0.03125;
  // see Figure 11 from paper - backup energy penalty is 750 pj
  static constexpr auto backup_energy_penalty = 0.75;
  // see Figure 10 from paper - backup time penalty is 35 cycles
  static constexpr uint64_t backup_time_penalty = 35;
  // see Figure 11 from paper - restore cost is 250 pj
  static constexpr auto recovery_energy_penalty = 0.25;
  // see Figure 10 from paper - recovery time penalty is 35 cycles
  static constexpr uint64_t restore_time_penalty = 35;
  // see Section V from paper - capacitor used in the system is 470 nF
  static constexpr double capacitance = 470e-9;
};
}
#endif //EH_SIM_ON_DEMAND_ALL_BACKUP_HPP
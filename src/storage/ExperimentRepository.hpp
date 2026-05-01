#pragma once

#include <optional>
#include <vector>

#include "ExperimentResult.hpp"

namespace qi::storage {

class DatabaseManager;

// Persistence for ExperimentResult. All operations are atomic
// (single-transaction) for write paths.
class ExperimentRepository {
public:
    explicit ExperimentRepository(DatabaseManager& db) : db_(db) {}

    // Saves an experiment as a single transaction. On success, populates
    // `experiment.id` and the `dbId` field of every surface/intersection,
    // then returns the new experiment id. Returns -1 on failure.
    int saveExperiment(qi::experiment::ExperimentResult& experiment);

    // Loads by id. Returns std::nullopt if no row matches.
    std::optional<qi::experiment::ExperimentResult> loadExperiment(int id);

    // All experiments, summary-only, ordered by id ascending.
    std::vector<qi::experiment::ExperimentSummary> listExperiments();

    // Removes by id. Cascades to surfaces and intersections via FK constraints.
    // Returns true if a row was deleted, false otherwise.
    bool deleteExperiment(int id);

private:
    DatabaseManager& db_;
};

}  // namespace qi::storage

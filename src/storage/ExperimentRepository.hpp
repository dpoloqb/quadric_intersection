#pragma once

#include <optional>
#include <vector>

#include "ExperimentResult.hpp"

namespace qi::storage {

class DatabaseManager;

/// Persistence for `qi::experiment::ExperimentResult`. All write operations
/// are atomic (single SQLite transaction).
///
/// Keep one repository per `DatabaseManager`; sharing across threads is
/// not supported (Qt SQL connections are per-thread).
class ExperimentRepository {
public:
    explicit ExperimentRepository(DatabaseManager& db) : db_(db) {}

    /// Save an experiment as a single transaction.
    ///
    /// On success, populates `experiment.id` and the `dbId` field of every
    /// surface and intersection in-place, then returns the new experiment id.
    /// Returns `-1` on failure (the DB is left untouched — rollback).
    int saveExperiment(qi::experiment::ExperimentResult& experiment);

    /// Load by id. Returns `std::nullopt` if no row matches.
    std::optional<qi::experiment::ExperimentResult> loadExperiment(int id);

    /// Summary list of all experiments, ordered by id ascending.
    std::vector<qi::experiment::ExperimentSummary> listExperiments();

    /// Remove by id. Cascades to `surfaces` and `intersections` rows via
    /// `ON DELETE CASCADE` foreign-key constraints. Returns `true` iff a row
    /// was actually removed.
    bool deleteExperiment(int id);

private:
    DatabaseManager& db_;
};

}  // namespace qi::storage

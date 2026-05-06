#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QModelIndex;
class QPushButton;
class QSortFilterProxyModel;
class QSqlQueryModel;
class QTableView;
class QTabWidget;
QT_END_NAMESPACE

namespace qi::storage {
class ExperimentRepository;
}

namespace qi::ui {

// Read/manage saved experiments. Two inner tabs:
//   - "Pairs" — every intersection row joined with its experiment and the
//     two surfaces, sortable / filterable via QSortFilterProxyModel.
//   - "Experiments" — one row per experiment; double-click opens a details
//     dialog summarizing the experiment.
//
// Toolbar:
//   - Delete: removes selected experiment (cascades via FK).
//   - Export CSV: writes the current pairs view (filtered + sorted) to a CSV.
class ResultsTab : public QWidget {
    Q_OBJECT
public:
    explicit ResultsTab(QWidget* parent = nullptr);

    void setRepository(qi::storage::ExperimentRepository* repo);
    void setDatabase(QSqlDatabase db);

    // For tests: write the current pairs view to a CSV at `path`. Returns
    // true on success.
    bool exportPairsToCsv(const QString& path) const;

    // Test helpers — expose row counts of the two inner models.
    int pairsRowCount() const;
    int experimentsRowCount() const;

public slots:
    void refresh();

signals:
    void experimentDeleted(int experimentId);
    void experimentEditRequested(int experimentId);

private slots:
    void onDeleteSelected();
    void onExportCsv();
    void onExperimentDoubleClicked(const QModelIndex& index);

private:
    QString currentlySelectedExperimentId() const;

    QTabWidget* innerTabs_;

    QTableView* pairsView_;
    QSqlQueryModel* pairsModel_;
    QSortFilterProxyModel* pairsProxy_;
    QLineEdit* pairsFilter_;

    QTableView* experimentsView_;
    QSqlQueryModel* experimentsModel_;

    QPushButton* deleteBtn_;
    QPushButton* exportBtn_;

    QSqlDatabase db_;
    qi::storage::ExperimentRepository* repo_ = nullptr;
};

}  // namespace qi::ui

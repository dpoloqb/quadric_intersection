#include "ResultsTab.hpp"

#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QTabWidget>
#include <QTableView>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

#include "ExperimentRepository.hpp"

namespace qi::ui {

namespace {

const char* const kPairsQuery = R"sql(
    SELECT
        intersections.id                                AS pair_id,
        experiments.created_at                          AS created_at,
        s1.type                                         AS type1,
        s2.type                                         AS type2,
        s1.triangulation_method                         AS tri1,
        s2.triangulation_method                         AS tri2,
        intersections.intersection_method               AS method,
        intersections.time_intersection_ms              AS time_ms,
        intersections.segments_count                    AS segments,
        intersections.polylines_count                   AS polylines,
        experiments.id                                  AS experiment_id
    FROM intersections
    JOIN experiments ON intersections.experiment_id = experiments.id
    JOIN surfaces    s1 ON intersections.surface1_id = s1.id
    JOIN surfaces    s2 ON intersections.surface2_id = s2.id
    ORDER BY intersections.id ASC
)sql";

const char* const kExperimentsQuery = R"sql(
    SELECT id, created_at, surfaces_count, notes
    FROM experiments
    ORDER BY id ASC
)sql";

void styleTable(QTableView* v) {
    v->setSelectionBehavior(QAbstractItemView::SelectRows);
    v->setSelectionMode(QAbstractItemView::SingleSelection);
    v->setAlternatingRowColors(true);
    v->horizontalHeader()->setStretchLastSection(true);
    v->verticalHeader()->setVisible(false);
}

}  // namespace

ResultsTab::ResultsTab(QWidget* parent) : QWidget(parent) {
    innerTabs_ = new QTabWidget;

    // ---- Pairs tab ----
    auto* pairsPage = new QWidget;
    pairsView_ = new QTableView;
    pairsModel_ = new QSqlQueryModel(this);
    pairsProxy_ = new QSortFilterProxyModel(this);
    pairsProxy_->setSourceModel(pairsModel_);
    pairsProxy_->setFilterKeyColumn(-1);  // all columns
    pairsProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    pairsView_->setModel(pairsProxy_);
    pairsView_->setSortingEnabled(true);
    styleTable(pairsView_);

    pairsFilter_ = new QLineEdit;
    pairsFilter_->setPlaceholderText(tr("filter (matches any column)"));

    auto* pairsLayout = new QVBoxLayout(pairsPage);
    pairsLayout->addWidget(pairsFilter_);
    pairsLayout->addWidget(pairsView_, 1);

    // ---- Experiments tab ----
    auto* experimentsPage = new QWidget;
    experimentsView_ = new QTableView;
    experimentsModel_ = new QSqlQueryModel(this);
    experimentsView_->setModel(experimentsModel_);
    experimentsView_->setSortingEnabled(false);
    styleTable(experimentsView_);

    auto* experimentsLayout = new QVBoxLayout(experimentsPage);
    experimentsLayout->addWidget(new QLabel(tr("Double-click a row for details")));
    experimentsLayout->addWidget(experimentsView_, 1);

    innerTabs_->addTab(pairsPage, tr("Intersection pairs"));
    innerTabs_->addTab(experimentsPage, tr("Experiments"));

    // ---- Toolbar ----
    deleteBtn_ = new QPushButton(tr("Delete selected"));
    exportBtn_ = new QPushButton(tr("Export CSV…"));
    auto* toolbar = new QHBoxLayout;
    toolbar->addWidget(deleteBtn_);
    toolbar->addWidget(exportBtn_);
    toolbar->addStretch(1);

    auto* main = new QVBoxLayout(this);
    main->addLayout(toolbar);
    main->addWidget(innerTabs_, 1);

    connect(pairsFilter_, &QLineEdit::textChanged, pairsProxy_,
            &QSortFilterProxyModel::setFilterFixedString);
    connect(deleteBtn_, &QPushButton::clicked, this, &ResultsTab::onDeleteSelected);
    connect(exportBtn_, &QPushButton::clicked, this, &ResultsTab::onExportCsv);
    connect(experimentsView_, &QTableView::doubleClicked, this,
            &ResultsTab::onExperimentDoubleClicked);
}

void ResultsTab::setRepository(qi::storage::ExperimentRepository* repo) { repo_ = repo; }

void ResultsTab::setDatabase(QSqlDatabase db) {
    db_ = std::move(db);
    refresh();
}

void ResultsTab::refresh() {
    if (!db_.isValid() || !db_.isOpen()) {
        pairsModel_->clear();
        experimentsModel_->clear();
        return;
    }
    pairsModel_->setQuery(kPairsQuery, db_);
    experimentsModel_->setQuery(kExperimentsQuery, db_);

    // Friendly column headers (set after every refresh — setQuery resets them).
    pairsModel_->setHeaderData(0, Qt::Horizontal, tr("id"));
    pairsModel_->setHeaderData(1, Qt::Horizontal, tr("created"));
    pairsModel_->setHeaderData(2, Qt::Horizontal, tr("type 1"));
    pairsModel_->setHeaderData(3, Qt::Horizontal, tr("type 2"));
    pairsModel_->setHeaderData(4, Qt::Horizontal, tr("tri 1"));
    pairsModel_->setHeaderData(5, Qt::Horizontal, tr("tri 2"));
    pairsModel_->setHeaderData(6, Qt::Horizontal, tr("method"));
    pairsModel_->setHeaderData(7, Qt::Horizontal, tr("time, ms"));
    pairsModel_->setHeaderData(8, Qt::Horizontal, tr("segments"));
    pairsModel_->setHeaderData(9, Qt::Horizontal, tr("polylines"));
    pairsModel_->setHeaderData(10, Qt::Horizontal, tr("experiment"));

    experimentsModel_->setHeaderData(0, Qt::Horizontal, tr("id"));
    experimentsModel_->setHeaderData(1, Qt::Horizontal, tr("created"));
    experimentsModel_->setHeaderData(2, Qt::Horizontal, tr("surfaces"));
    experimentsModel_->setHeaderData(3, Qt::Horizontal, tr("notes"));
}

int ResultsTab::pairsRowCount() const { return pairsProxy_->rowCount(); }
int ResultsTab::experimentsRowCount() const { return experimentsModel_->rowCount(); }

QString ResultsTab::currentlySelectedExperimentId() const {
    // Prefer experiments tab if the user is there; otherwise fall back to
    // experiment_id from the selected pairs row.
    if (innerTabs_->currentIndex() == 1) {
        const auto idx = experimentsView_->currentIndex();
        if (idx.isValid()) {
            return experimentsModel_->data(experimentsModel_->index(idx.row(), 0)).toString();
        }
    } else {
        const auto idx = pairsView_->currentIndex();
        if (idx.isValid()) {
            const auto src = pairsProxy_->mapToSource(idx);
            return pairsModel_->data(pairsModel_->index(src.row(), 10)).toString();
        }
    }
    return {};
}

void ResultsTab::onDeleteSelected() {
    const QString idStr = currentlySelectedExperimentId();
    if (idStr.isEmpty() || repo_ == nullptr) return;
    bool ok = false;
    const int id = idStr.toInt(&ok);
    if (!ok) return;

    const auto button = QMessageBox::question(
        this, tr("Delete experiment"),
        tr("Delete experiment %1 and all its rows?").arg(id),
        QMessageBox::Yes | QMessageBox::No);
    if (button != QMessageBox::Yes) return;

    if (repo_->deleteExperiment(id)) {
        emit experimentDeleted(id);
        refresh();
    }
}

void ResultsTab::onExportCsv() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export pairs to CSV"), QString(), tr("CSV files (*.csv)"));
    if (path.isEmpty()) return;
    if (!exportPairsToCsv(path)) {
        QMessageBox::warning(this, tr("Export failed"),
                             tr("Could not write to %1").arg(path));
    }
}

bool ResultsTab::exportPairsToCsv(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);

    const int cols = pairsProxy_->columnCount();
    QStringList header;
    for (int c = 0; c < cols; ++c) {
        header << pairsProxy_->headerData(c, Qt::Horizontal).toString();
    }
    out << header.join(',') << '\n';

    const int rows = pairsProxy_->rowCount();
    for (int r = 0; r < rows; ++r) {
        QStringList cells;
        for (int c = 0; c < cols; ++c) {
            QString cell = pairsProxy_->index(r, c).data().toString();
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace('"', "\"\"");
                cell = '"' + cell + '"';
            }
            cells << cell;
        }
        out << cells.join(',') << '\n';
    }
    return true;
}

void ResultsTab::onExperimentDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    const int row = index.row();
    const int id = experimentsModel_->data(experimentsModel_->index(row, 0)).toInt();
    const QString createdAt = experimentsModel_->data(experimentsModel_->index(row, 1)).toString();
    const int surfacesCount = experimentsModel_->data(experimentsModel_->index(row, 2)).toInt();
    const QString notes = experimentsModel_->data(experimentsModel_->index(row, 3)).toString();

    QString details;
    QTextStream s(&details);
    s << "Experiment #" << id << "\n"
      << "Created: " << createdAt << "\n"
      << "Surfaces: " << surfacesCount << "\n"
      << "Notes: " << (notes.isEmpty() ? QString("—") : notes) << "\n\n";

    if (repo_ != nullptr) {
        auto exp = repo_->loadExperiment(id);
        if (exp.has_value()) {
            s << "Bbox: ["
              << exp->bbox.min().x() << ", " << exp->bbox.min().y() << ", "
              << exp->bbox.min().z() << "] – ["
              << exp->bbox.max().x() << ", " << exp->bbox.max().y() << ", "
              << exp->bbox.max().z() << "]\n\n";
            s << "Surfaces:\n";
            for (const auto& sr : exp->surfaces) {
                s << "  " << sr.indexInExperiment << ": " << sr.type
                  << " (" << sr.triangulationMethod << ", "
                  << sr.trianglesCount << " triangles, "
                  << QString::number(sr.timeTriangulationMs, 'f', 2) << " ms)\n";
            }
            s << "\nIntersections:\n";
            for (const auto& isec : exp->intersections) {
                s << "  (" << isec.surface1Index << ", " << isec.surface2Index
                  << ") via " << isec.intersectionMethod
                  << ": " << isec.segmentsCount << " segments, "
                  << isec.polylinesCount << " polylines, "
                  << QString::number(isec.timeIntersectionMs, 'f', 3) << " ms\n";
            }
        }
    }

    QDialog dlg(const_cast<ResultsTab*>(this));
    dlg.setWindowTitle(tr("Experiment %1").arg(id));
    auto* layout = new QVBoxLayout(&dlg);
    auto* text = new QTextEdit;
    text->setReadOnly(true);
    text->setPlainText(details);
    layout->addWidget(text);
    auto* close = new QPushButton(tr("Close"));
    connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(close);
    dlg.resize(500, 400);
    dlg.exec();
}

}  // namespace qi::ui

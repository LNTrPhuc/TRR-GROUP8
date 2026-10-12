#include "MainWindow.h"
#include "PathPreview.h"
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QHeaderView>
#include <QFrame>
#include <QTabWidget>
#include <QSplitter>
#include <QSignalBlocker>
#include <QShortcut>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextFormat>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QListView>
#include <QStyledItemDelegate>
#include <QFileDialog>
#include <QFile>
#include <algorithm>
#include <limits>
#include <cmath>

class PopupItemDelegate : public QStyledItemDelegate {
public:
    explicit PopupItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(42);
        return size;
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    simulator = new DijkstraSimulator();
    playTimer = new QTimer(this);
    connect(playTimer, &QTimer::timeout, this, &MainWindow::advancePlayback);
    setupUI();
    simulator->loadTemplate(0);
    updateUI();
}
MainWindow::~MainWindow() { delete simulator; }

void MainWindow::setupUI() {
    const QSize available = screen()->availableGeometry().size();
    setMinimumSize(QSize(480,360).boundedTo(available));
    resize(QSize(1280,820).boundedTo(available * 0.9));
    setWindowTitle("Dijkstra — Mô phỏng đường đi ngắn nhất");
    setStyleSheet(R"(
        QWidget { font-family: 'Segoe UI'; font-size: 13px; color: #334155; }
        QMainWindow, QWidget#central { background: #f1f5f9; }
        QFrame#header, QFrame#panel { background: #ffffff; border: 1px solid #dbe3ef; border-radius: 10px; }
        QLabel { background: transparent; border: none; }
        QLabel#brand { font-size: 25px; font-weight: 700; color: #0f172a; }
        QLabel#sectionTitle { font-size: 14px; font-weight: 600; }
        QLabel#metric { background: #f1f5f9; border-radius: 6px; padding: 8px; color: #334155; }
        QLabel#statusLbl { background: #eff6ff; border: 1px solid #bfdbfe; border-radius: 8px; padding: 12px; color: #1e40af; }
        QComboBox { background: white; border: 1px solid #cbd5e1; border-radius: 6px; padding: 8px 12px; min-height: 20px; }
        QComboBox QAbstractItemView { background: white; selection-background-color: #dbeafe; selection-color: #0f172a; }
        QPushButton { background: white; border: 1px solid #cbd5e1; border-radius: 6px; padding: 9px 12px; font-weight: 600; }
        QPushButton:hover { background: #eff6ff; border-color: #93c5fd; }
        QPushButton:pressed { background: #dbeafe; }
        QPushButton:disabled { color: #9ca9a3; background: #f5f7f6; border-color: #e4e9e6; }
        QPushButton#playBtn { background: #2563eb; color: white; border-color: #2563eb; }
        QPushButton#playBtn:hover { background: #1d4ed8; }
        QPushButton#playBtn:disabled { background: #cbd5e1; border-color: #cbd5e1; }
        QSlider::groove:horizontal { height: 4px; background: #e2e8f0; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #3b82f6; }
        QSlider::handle:horizontal { width: 14px; margin: -5px 0; background: #2563eb; border-radius: 7px; }
        QTableWidget, QListWidget, QTextEdit { background: white; alternate-background-color: #f8fafc; border: none; selection-background-color: #dbeafe; }
        QTableWidget::item, QListWidget::item { padding: 6px; }
        QHeaderView::section { background: #f1f5f9; color: #64748b; padding: 9px; border: none; font-weight: 600; }
        QTabWidget::pane { background: white; border: 1px solid #dbe3ef; border-radius: 8px; }
        QTabBar::tab { background: transparent; color: #64748b; padding: 11px 14px; border-bottom: 2px solid transparent; }
        QTabBar::tab:selected { color: #2563eb; border-bottom: 2px solid #2563eb; font-weight: 600; }
        QSplitter::handle { background: transparent; }
        QScrollBar:vertical { background: #f1f5f9; width: 10px; }
        QScrollBar::handle:vertical { background: #cbd5e1; min-height: 24px; border-radius: 4px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");
    auto *viewport = new QScrollArea(this);
    viewport->setObjectName("windowViewport");
    viewport->setWidgetResizable(true); viewport->setFrameShape(QFrame::NoFrame);
    auto *central = new QWidget; central->setObjectName("central");
    viewport->setWidget(central); setCentralWidget(viewport);
    auto *root = new QVBoxLayout(central); root->setContentsMargins(24, 20, 24, 16); root->setSpacing(16);
    root->setSizeConstraint(QLayout::SetMinimumSize);
    auto *header = new QFrame; header->setObjectName("header");
    auto *hl = new QHBoxLayout(header); hl->setContentsMargins(20, 16, 20, 16);
    auto *brandLayout = new QVBoxLayout; brandLayout->setSpacing(3);
    auto *brand = new QLabel("Dijkstra"); brand->setObjectName("brand");
    brandLayout->addWidget(brand); hl->addLayout(brandLayout); hl->addStretch();
    codeToggle = new QPushButton("Mở code thuật toán");
    codeToggle->setCheckable(true);
    hl->addWidget(codeToggle); root->addWidget(header);
    auto *split = new QSplitter(Qt::Horizontal); split->setChildrenCollapsible(false); split->setHandleWidth(14);
    auto *left = new QWidget; auto *ll = new QHBoxLayout(left); ll->setContentsMargins(0,0,0,0); setupLeftPanel(ll);
    auto *right = new QWidget; auto *rl = new QHBoxLayout(right); rl->setContentsMargins(0,0,0,0); setupRightPanel(rl);
    left->setMinimumWidth(570); right->setMinimumWidth(310); split->addWidget(left); split->addWidget(right); split->setStretchFactor(0, 3); split->setStretchFactor(1, 1); split->setSizes({850,350}); root->addWidget(split,1);
    auto *foot = new QLabel("Kéo vùng trắng: chọn nhiều · Ctrl: thêm/bớt chọn · Backspace: xóa · Bấm đúp đỉnh/trọng số: sửa · Chọn đỉnh rồi kéo trái: nối cạnh · Alt+kéo: di chuyển · Lăn chuột: zoom · Kéo giữa: dịch khung nhìn · Space: chạy/dừng"); foot->setStyleSheet("color: #64748b; font-size: 11px;"); foot->setWordWrap(true); root->addWidget(foot);
    auto *space = new QShortcut(QKeySequence(Qt::Key_Space), this); connect(space, &QShortcut::activated, this, &MainWindow::onPlayPause);
    auto *next = new QShortcut(QKeySequence(Qt::Key_Right), this); connect(next, &QShortcut::activated, this, &MainWindow::onNextStep);
    auto *prev = new QShortcut(QKeySequence(Qt::Key_Left), this); connect(prev, &QShortcut::activated, this, &MainWindow::onPrevStep);
}
void MainWindow::setupLeftPanel(QHBoxLayout *mainLayout) {
    auto *panel = new QWidget; auto *layout = new QVBoxLayout(panel); layout->setContentsMargins(0,0,0,0); layout->setSpacing(12);
    auto *toolbar = new QFrame; toolbar->setObjectName("panel"); auto *tl = new QHBoxLayout(toolbar); tl->setContentsMargins(14,10,14,10);
    comboTemplates = new QComboBox; comboTemplates->addItems({"Cơ bản · 6 đỉnh", "Tuyến tính", "Cạnh song song", "Bẫy trọng số", "Đồ thị lưới", "Bảng trắng"}); comboTemplates->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed); tl->addWidget(comboTemplates,1); layout->addWidget(toolbar);
    comboTemplates->setObjectName("graphTemplates");
    comboTemplates->setPlaceholderText("Đồ thị tùy chỉnh");
    auto *edit = new QPushButton("Insert"); edit->setObjectName("editGraph"); tl->addWidget(edit);
    auto *random = new QPushButton("Tạo ngẫu nhiên"); random->setObjectName("randomGraph"); tl->addWidget(random);
    edit->setToolTip("Nhập đồ thị từ file TXT (tối đa 30 đỉnh)");
    connect(edit, &QPushButton::clicked, this, &MainWindow::onImportText);
    connect(random, &QPushButton::clicked, this, &MainWindow::onRandomGraph);
    connect(comboTemplates,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&MainWindow::onGraphTemplateChanged);
    graphWidget = new GraphWidget(simulator);
    auto *options=new QHBoxLayout;
    options->addWidget(new QLabel("Nguồn")); sourceSelector=new QComboBox;sourceSelector->setObjectName("sourceSelector");options->addWidget(sourceSelector,1);
    options->addWidget(new QLabel("Đích")); targetSelector=new QComboBox;targetSelector->setObjectName("targetSelector");options->addWidget(targetSelector,1);
    edgeKindSelector=new QComboBox;edgeKindSelector->setObjectName("edgeKindSelector");
    edgeKindSelector->addItems({"Đồ thị: vô hướng","Đồ thị: có hướng","Đồ thị: hỗn hợp"});
    edgeKindSelector->setToolTip(
        "Định dạng toàn bộ đồ thị hiện tại.\n\n"
        "Có hướng: cạnh vô hướng nhận chiều\n"
        "theo thứ tự tạo đỉnh.\n\n"
        "Vô hướng: bỏ hướng từng cạnh,\n"
        "giữ riêng mọi cạnh và trọng số.\n\n"
        "Hỗn hợp: giữ cạnh hiện tại,\n"
        "cạnh mới mặc định có hướng;\n"
        "chuột phải cạnh hoặc trọng số để xóa hướng.\n\n"
        "Xóa hướng của một cạnh trong đồ thị có hướng\n"
        "sẽ chuyển chế độ sang hỗn hợp.");
    options->addWidget(edgeKindSelector,1);layout->addLayout(options);
    connect(sourceSelector,&QComboBox::currentIndexChanged,this,[this](int i){
        if(i<0) return;
        playTimer->stop();simulator->startNode=sourceSelector->itemData(i).toInt();simulator->reset();graphWidget->animateStep(0,true);updateUI();
    });
    connect(targetSelector,&QComboBox::currentIndexChanged,this,[this](int i){
        if(i<0) return;
        playTimer->stop();simulator->targetNode=targetSelector->itemData(i).toInt();simulator->reset();graphWidget->animateStep(0,true);updateUI();
    });
    connect(edgeKindSelector,&QComboBox::currentIndexChanged,this,[this](int i){
        if(i<0) return;
        playTimer->stop();
        simulator->setGraphType(static_cast<DijkstraSimulator::GraphType>(i));
        markCustomGraph();
    });
    auto *viewControls=new QHBoxLayout;viewControls->addStretch();
    for(const auto &entry: {std::pair<QString,qreal>{"−",1/1.2},{"+",1.2},{"100%",0}}) {
        auto *button=new QPushButton(entry.first);button->setObjectName(entry.second==0?"resetZoom":entry.second>1?"zoomIn":"zoomOut");
        if(entry.second==0) connect(graphWidget,&GraphWidget::zoomChanged,button,[button](qreal factor){button->setText(QString::number(qRound(factor*100))+"%");});
        button->setToolTip(entry.second==0?"Đặt lại khung nhìn":"Thu/phóng đồ thị; cũng có thể lăn chuột");
        viewControls->addWidget(button);connect(button,&QPushButton::clicked,this,[this,factor=entry.second]{if(factor==0) graphWidget->resetView();else graphWidget->zoomBy(factor);});
    }

    connect(graphWidget, &GraphWidget::interactionStarted, this, [this] { playTimer->stop(); updateUI(); });
    connect(graphWidget, &GraphWidget::graphEdited, this, &MainWindow::markCustomGraph);
    auto *graphScroll = new QScrollArea; graphScroll->setWidgetResizable(true); graphScroll->setWidget(graphWidget);
    graphScroll->setObjectName("graphViewport"); graphScroll->setMinimumSize(400,260); graphScroll->setFrameShape(QFrame::NoFrame);
    layout->addWidget(graphScroll,1);
    auto *legend = new QLabel("<span style=\"color:#2563eb\">● Đang xét</span> &nbsp; <span style=\"color:#0f766e\">✓ Đã chốt</span> &nbsp; <span style=\"color:#d97706\">● Đang kiểm tra</span> &nbsp; <span style=\"color:#7c3aed\">◎ Đích đến</span><br><span style=\"color:#64748b\">━ Tuyến tạm thời</span> &nbsp; <span style=\"color:#db2777\">━ Đường đi ngắn nhất</span>"); legend->setObjectName("graphLegend"); legend->setStyleSheet("color:#64748b; font-size:12px; padding: 2px 4px;"); legend->setWordWrap(true); legend->setMinimumHeight(40); viewControls->insertWidget(0,legend,1); layout->addLayout(viewControls);
    lblExplanation = new QLabel; lblExplanation->setObjectName("statusLbl"); lblExplanation->setWordWrap(true); lblExplanation->setMinimumHeight(64); layout->addWidget(lblExplanation);
    auto *controlsPanel = new QFrame; controlsPanel->setObjectName("panel"); auto *cl = new QVBoxLayout(controlsPanel); cl->setContentsMargins(12,12,12,12);
    auto *buttons = new QHBoxLayout; btnPrev = new QPushButton("← Quay lại"); btnPlay = new QPushButton("Chạy tự động"); btnNext = new QPushButton("Bước tiếp →"); auto *btnReset = new QPushButton("Đặt lại"); btnPlay->setObjectName("playBtn");
    for (auto *b : {btnPrev,btnPlay,btnNext,btnReset}) { buttons->addWidget(b); }
    cl->addLayout(buttons);
    auto *speed = new QHBoxLayout; speed->addWidget(new QLabel("Tốc độ")); speed->addWidget(new QLabel("Chậm")); speedSlider = new QSlider(Qt::Horizontal); speedSlider->setRange(100,2000); speedSlider->setValue(1100); speedSlider->setTickPosition(QSlider::TicksBelow); speedSlider->setTickInterval(475); speed->addWidget(speedSlider,1); speed->addWidget(new QLabel("Nhanh")); cl->addLayout(speed); layout->addWidget(controlsPanel);
    speedSlider->setObjectName("speedSlider");
    speedSlider->setToolTip("Tăng tốc theo cấp số nhân; thời gian chạy được cân bằng theo số bước. Kéo hết sang phải để nhận kết quả tức thì");
    connect(speedSlider,&QSlider::valueChanged,this,[this](int value){
        if (value == speedSlider->maximum()) showInstantResult();
        else { graphWidget->setInstantResult(false); }
    });
    connect(btnPlay,&QPushButton::clicked,this,&MainWindow::onPlayPause); connect(btnNext,&QPushButton::clicked,this,&MainWindow::onNextStep); connect(btnPrev,&QPushButton::clicked,this,&MainWindow::onPrevStep); connect(btnReset,&QPushButton::clicked,this,&MainWindow::onReset); mainLayout->addWidget(panel);
}
void MainWindow::setupRightPanel(QHBoxLayout *mainLayout) {
    auto *panel = new QWidget; auto *layout = new QVBoxLayout(panel); layout->setContentsMargins(0,0,0,0); layout->setSpacing(12);
    auto *metrics = new QFrame; metrics->setObjectName("panel"); auto *ml = new QGridLayout(metrics); ml->setContentsMargins(12,12,12,12);
    lblStatus = new QLabel; lblStep = new QLabel; lblNodes = new QLabel; lblEdges = new QLabel; lblPath = new QLabel; lblPath->setWordWrap(true);
    for(auto *label : {lblStatus,lblStep,lblNodes,lblEdges,lblPath}) label->setObjectName("metric");
    ml->addWidget(lblStatus,0,0); ml->addWidget(lblStep,0,1); ml->addWidget(lblNodes,1,0); ml->addWidget(lblEdges,1,1); ml->addWidget(lblPath,2,0,1,2); layout->addWidget(metrics);
    detailTabs = new QTabWidget;
    detailTabs->setObjectName("detailTabs");
    auto *data = new QWidget; auto *dl = new QVBoxLayout(data); dl->setContentsMargins(10,12,10,10);
    auto *heading = new QLabel("Khoảng cách từ đỉnh xuất phát"); heading->setWordWrap(true); heading->setObjectName("sectionTitle"); dl->addWidget(heading);
    distTable = new QTableWidget(0,3); distTable->setHorizontalHeaderLabels({"Đỉnh","Khoảng cách","Trước"}); distTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); distTable->verticalHeader()->hide(); distTable->verticalHeader()->setDefaultSectionSize(36); distTable->setShowGrid(false); distTable->setAlternatingRowColors(true); distTable->setEditTriggers(QAbstractItemView::NoEditTriggers); distTable->setSelectionMode(QAbstractItemView::NoSelection); dl->addWidget(distTable,3);
    distTable->setObjectName("distanceTable");
    distTable->setMouseTracking(true);distTable->viewport()->setMouseTracking(true);
    pathPreview=new PathPreview(simulator,this);
    distTable->viewport()->installEventFilter(this);installEventFilter(this);
    auto *qh = new QLabel("Hàng đợi ưu tiên"); qh->setObjectName("sectionTitle"); dl->addWidget(qh); auto *hint = new QLabel("Đỉnh có khoảng cách nhỏ nhất được lấy trước."); hint->setWordWrap(true); hint->setStyleSheet("color:#64748b; font-size:11px;"); dl->addWidget(hint); pqList = new QListWidget; pqList->setMinimumHeight(70); pqList->setMaximumHeight(130); dl->addWidget(pqList,1); detailTabs->addTab(data,"Dữ liệu");
    codeViewer = new QTextEdit; codeViewer->setReadOnly(true); codeViewer->setLineWrapMode(QTextEdit::WidgetWidth); codeViewer->setFont(QFont("Consolas",10));
    codeViewer->setPlainText("01  // Dijkstra\n02  dist = ∞; prev = -1; dist[start] = 0\n03  queue.push(0, start)\n04  while queue is not empty:\n05    d, u = queue.pop_min()\n06    if visited[u]: continue\n07    visited[u] = true\n08    if target >= 0 && u == target: return path(prev)\n09    for (v, w) in edges[u]:\n10      if !visited[v] && dist[u]+w < dist[v]:\n11        dist[v]=dist[u]+w; prev[v]=u; queue.push(dist[v],v)\n12  return dist, prev");
    codeViewer->setStyleSheet("QTextEdit { font-family: Consolas; font-size: 13px; background: #0f172a; color: #dbeafe; padding: 12px; border-radius: 8px; selection-background-color: #334155; }");
    codeViewer->document()->setDocumentMargin(4);
    auto *codePanel = new QWidget;
    auto *codeLayout = new QVBoxLayout(codePanel);
    codeLayout->setContentsMargins(10,12,10,10);
    auto *codeHeading = new QLabel("Thuật toán Dijkstra"); codeHeading->setObjectName("sectionTitle");
    auto *codeHint = new QLabel("Dòng màu xanh là thao tác vừa thực thi. Code tự cuộn theo từng bước.");
    codeHint->setWordWrap(true); codeHint->setStyleSheet("color:#64748b; font-size:12px;");
    codeLayout->addWidget(codeHeading); codeLayout->addWidget(codeHint); codeLayout->addWidget(codeViewer,1);
    detailTabs->addTab(codePanel,"Code thuật toán");
    connect(codeToggle, &QPushButton::toggled, this, [this](bool visible) {
        detailTabs->setCurrentIndex(visible ? 1 : 0);
    });
    connect(detailTabs, &QTabWidget::currentChanged, this, [this](int index) {
        hidePathPreview();
        const QSignalBlocker blocker(codeToggle);
        codeToggle->setChecked(index == 1);
        codeToggle->setText(index == 1 ? "Xem dữ liệu" : "Mở code thuật toán");
    });
    layout->addWidget(detailTabs,1); mainLayout->addWidget(panel);
}
void MainWindow::onGraphTemplateChanged(int index) {
    if(index < 0 || index > 5) return;
    hidePathPreview();
    playTimer->stop();
    simulator->loadTemplate(index);
    graphWidget->resetLayout();
    graphWidget->animateStep(0,true);
    updateUI();
}
void MainWindow::markCustomGraph() {
    hidePathPreview();
    const QSignalBlocker blocker(comboTemplates);
    comboTemplates->setCurrentIndex(-1);
    graphWidget->animateStep(0, true);
    updateUI();
}

void MainWindow::onRandomGraph() {
    playTimer->stop(); updateUI();
    QDialog dialog(this); dialog.setWindowTitle("Tạo đồ thị ngẫu nhiên");
    dialog.setMinimumWidth(470);
    dialog.setStyleSheet(R"(
        QDialog { background: #f8fafc; }
        QFrame#randomCard { background: white; border: 1px solid #dbe3ef; border-radius: 16px; }
        QLabel { color: #334155; background: transparent; }
        QSpinBox, QComboBox { background: white; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 8px; padding: 8px 12px; min-height: 24px; }
        QSpinBox:focus, QComboBox:focus { border: 1px solid #2563eb; }
        QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 26px; background: #f1f5f9; border-left: 1px solid #cbd5e1; border-top-right-radius: 8px; }
        QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 26px; background: #f1f5f9; border-left: 1px solid #cbd5e1; border-bottom-right-radius: 8px; }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: #dbeafe; }
        QSpinBox::up-arrow { image: url(:/icons/up.svg); width: 12px; height: 12px; }
        QSpinBox::down-arrow { image: url(:/icons/down.svg); width: 12px; height: 12px; }
        QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 30px; border: none; }
        QComboBox::down-arrow { image: url(:/icons/down.svg); width: 14px; height: 14px; }
        QComboBox QAbstractItemView { background: #ffffff; color: #0f172a; selection-background-color: #2563eb; selection-color: white; border: 1px solid #cbd5e1; outline: none; padding: 6px; }
        QComboBox QAbstractItemView::item { min-height: 32px; padding: 6px; }
        QPushButton { background: white; color: #334155; border: 1px solid #cbd5e1; border-radius: 8px; padding: 10px 16px; }
        QPushButton:hover { background: #eff6ff; border-color: #2563eb; }
        QPushButton#generateButton { background: #2563eb; color: white; border-color: #2563eb; }
        QPushButton#generateButton:hover { background: #1d4ed8; }
    )");
    auto *outer = new QVBoxLayout(&dialog); outer->setContentsMargins(16,16,16,16);
    auto *card = new QFrame; card->setObjectName("randomCard"); outer->addWidget(card);
    auto *layout = new QVBoxLayout(card); layout->setContentsMargins(24,24,24,24); layout->setSpacing(18);
    auto *title = new QLabel("Tạo đồ thị ngẫu nhiên"); title->setStyleSheet("font-size:20px; font-weight:700; color:#0f172a;"); layout->addWidget(title);
    auto *form = new QFormLayout; form->setSpacing(12);
    auto *count = new QSpinBox; count->setObjectName("randomNodeCount"); count->setRange(2, DijkstraSimulator::MaxRandomNodes); count->setValue(8);
    auto *style = new QComboBox; style->setObjectName("randomLayout");
    style->setView(new QListView);
    style->setItemDelegate(new PopupItemDelegate(style));
    qobject_cast<QListView*>(style->view())->setUniformItemSizes(true);
    style->view()->setStyleSheet("QListView { background: white; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 8px; padding: 6px; outline: none; } QListView::item { padding: 8px 12px; min-height: 24px; border-radius: 5px; } QListView::item:selected { background: #2563eb; color: white; } QListView::item:hover { background: #dbeafe; color: #1e40af; }");
    style->addItem("Hình tròn", static_cast<int>(DijkstraSimulator::Layout::Circle));
    style->addItem("Hình vuông", static_cast<int>(DijkstraSimulator::Layout::Square));
    style->addItem("Lưới", static_cast<int>(DijkstraSimulator::Layout::Grid));
    style->addItem("Tự do", static_cast<int>(DijkstraSimulator::Layout::Random));
    auto *type = new QComboBox; type->setObjectName("randomGraphType");
    type->addItems({"Vô hướng", "Có hướng"});
    type->setCurrentIndex(simulator->graphType == DijkstraSimulator::GraphType::Undirected ? 0 : 1);
    auto *density = new QSpinBox; density->setObjectName("randomDensity"); density->setRange(0, 100); density->setValue(20); density->setSuffix(" %");
    auto *maxWeight = new QSpinBox; maxWeight->setObjectName("randomMaxWeight"); maxWeight->setRange(1, DijkstraSimulator::MaxEdgeWeight); maxWeight->setValue(20);
    form->addRow("Loại đồ thị", type); form->addRow("Số đỉnh", count); form->addRow("Bố trí đỉnh", style); form->addRow("Xác suất thêm cạnh", density); form->addRow("Trọng số tối đa", maxWeight);
    layout->addLayout(form);
    auto *hint = new QLabel("Đồ thị tạo ra bảo đảm nguồn ban đầu đi tới mọi đỉnh, kể cả đồ thị có hướng. Trọng số ngẫu nhiên từ 1 đến mức tối đa.\nKiểu bố trí quyết định vị trí đỉnh; các cạnh được tạo ngẫu nhiên.\n0%: chỉ tạo cây nối các đỉnh. 100%: nối mọi cặp đỉnh.\nNếu đổi nguồn hoặc xóa/đổi chiều cạnh, có thể có đỉnh không tới được. Lăn chuột để zoom.");
    hint->setWordWrap(true); hint->setStyleSheet("color:#475569; line-height:1.5;"); layout->addWidget(hint);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText("Tạo đồ thị"); buttons->button(QDialogButtonBox::Ok)->setObjectName("generateButton"); buttons->button(QDialogButtonBox::Cancel)->setText("Hủy"); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        simulator->graphType=type->currentIndex()==0 ? DijkstraSimulator::GraphType::Undirected : DijkstraSimulator::GraphType::Directed;
        if (simulator->generateRandomGraph(count->value(),
            static_cast<DijkstraSimulator::Layout>(style->currentData().toInt()), density->value(), maxWeight->value())) {
            graphWidget->resetLayout(); markCustomGraph();
        }
    }
}
void MainWindow::onImportText() {
    playTimer->stop(); updateUI();
    const QString path = QFileDialog::getOpenFileName(this, "Chọn file đồ thị TXT", QString(), "File văn bản (*.txt)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Không đọc được file", file.errorString()); return;
    }
    QString text = QString::fromUtf8(file.readAll());
    if (text.startsWith(QChar(0xfeff))) text.remove(0, 1);
    std::string error;
    if (!simulator->importText(text.toStdString(), error)) {
        QMessageBox::warning(this, "Không thể nhập đồ thị", QString::fromStdString(error)); return;
    }
    graphWidget->resetLayout(); markCustomGraph();
}

void MainWindow::showInstantResult() {
    playTimer->stop();
    simulator->runToEnd();
    graphWidget->setInstantResult(true);
    updateUI();
}

double MainWindow::playbackDuration() const {
    const double t=double(speedSlider->value()-speedSlider->minimum())/(speedSlider->maximum()-speedSlider->minimum());
    return 60.0*std::pow(1.0/75.0,t);
}
void MainWindow::onPlayPause() {
    if(graphWidget->isEditing() || simulator->isFinished) return;
    if(speedSlider->value()==speedSlider->maximum()) {showInstantResult();return;}
    if(playTimer->isActive()) playTimer->stop();
    else {
        // Count the actual simulation work on a fresh copy, without retaining
        // duplicate history. The chosen speed sets total duration for any size.
        DijkstraSimulator probe;
        probe.graph=simulator->graph;
        probe.startNode=simulator->startNode;
        probe.targetNode=simulator->targetNode;
        probe.graphType=simulator->graphType;
        probe.reset();playbackSteps=0;
        while(!probe.isFinished) {probe.step();probe.history.clear();++playbackSteps;}
        stepBudget=0;playbackClock.start();playTimer->start(16);
    }
    updateUI();
}
void MainWindow::advancePlayback() {
    stepBudget+=playbackClock.restart()/1000.0*playbackSteps/playbackDuration();
    const int count=static_cast<int>(stepBudget);
    if(count<1) return;
    stepBudget-=count;
    for(int i=0;i<count && !simulator->isFinished;++i) simulator->step();
    graphWidget->setInstantResult(false);
    graphWidget->animateStep(count>1?0:std::clamp(static_cast<int>(1000*playbackDuration()/std::max(1,playbackSteps)),16,350));
    if(simulator->isFinished) playTimer->stop();
    updateUI();
}
void MainWindow::onNextStep() {
    if(graphWidget->isEditing()) return;
    if(speedSlider->value()==speedSlider->maximum()) {
        showInstantResult();
        return;
    }
    graphWidget->setInstantResult(false);
    if(!simulator->isFinished) {
        simulator->step();
        graphWidget->animateStep(std::clamp(static_cast<int>(playbackDuration()*5),40,350));
    }
    if(simulator->isFinished) playTimer->stop();
    updateUI();
}

void MainWindow::onPrevStep() {
    if(graphWidget->isEditing()) return;
    playTimer->stop();
    graphWidget->setInstantResult(false);
    simulator->stepBack();
    graphWidget->animateStep(400);
    updateUI();
}

void MainWindow::onReset() {
    playTimer->stop();
    simulator->reset();
    graphWidget->animateStep(0,true);
    updateUI();
}
void MainWindow::updateUI() {
    graphWidget->update(); const int count = static_cast<int>(simulator->graph.size());
    const QSignalBlocker sourceBlock(sourceSelector),targetBlock(targetSelector),typeBlock(edgeKindSelector);
    edgeKindSelector->setCurrentIndex(static_cast<int>(simulator->graphType));
    bool rebuildSelectors=sourceSelector->count()!=count || targetSelector->count()!=count+1;
    for(int i=0;!rebuildSelectors && i<count;++i)
        rebuildSelectors=sourceSelector->itemText(i)!=QString::fromStdString(simulator->graph[i].name);
    if(rebuildSelectors) {
        sourceSelector->clear();targetSelector->clear();targetSelector->addItem("Mọi đỉnh",-1);
        for(const auto &n:simulator->graph) {
            sourceSelector->addItem(QString::fromStdString(n.name),n.id);
            targetSelector->addItem(QString::fromStdString(n.name),n.id);
        }
    }
    int edges=0;
    for(const auto &n:simulator->graph) {
        for(const auto &e:n.edges) if(e.directed || n.id<e.target) ++edges;
    }
    sourceSelector->setCurrentIndex(sourceSelector->findData(simulator->startNode));
    targetSelector->setCurrentIndex(targetSelector->findData(simulator->targetNode));
    sourceSelector->setEnabled(count>0);targetSelector->setEnabled(count>0);
    distTable->setRowCount(count);
    for(int i=0;i<count;++i) {
        const QString distance = simulator->dist[i]==std::numeric_limits<int>::max() ? QString::fromUtf8("∞") : QString::number(simulator->dist[i]);
        const QString previous = simulator->prev[i]<0 ? "—" : QString::fromStdString(simulator->graph[simulator->prev[i]].name);
        const QStringList values{QString::fromStdString(simulator->graph[i].name),distance,previous};
        for(int c=0;c<3;++c) { auto *item = distTable->item(i,c); if(!item) { item = new QTableWidgetItem; distTable->setItem(i,c,item); } item->setText(values[c]); item->setTextAlignment(Qt::AlignCenter); item->setBackground(i==simulator->currentNode ? QColor("#dbeafe") : QColor(Qt::transparent)); item->setForeground(simulator->visited[i] ? QColor("#0f766e") : QColor("#475569")); }
    }
    pqList->clear(); auto queue = simulator->pqState; std::sort(queue.begin(),queue.end(),[](const PQElement&a,const PQElement&b){return a.dist==b.dist ? a.vertex<b.vertex : a.dist<b.dist;});
    for(const auto &e:queue) { auto *item = new QListWidgetItem(QString("%1     ·     Khoảng cách %2%3").arg(QString::fromStdString(simulator->graph[e.vertex].name)).arg(e.dist).arg(simulator->visited[e.vertex] ? " (đã chốt)" : "")); if(simulator->visited[e.vertex]) item->setForeground(QColor("#94a198")); pqList->addItem(item); }
    if(queue.empty()) { auto *item = new QListWidgetItem("Hàng đợi trống"); item->setFlags(Qt::NoItemFlags); pqList->addItem(item); }
    lblExplanation->setText(QString::fromStdString(simulator->getCurrentExplanation())); lblStep->setText(QString("Bước %1").arg(simulator->history.size())); lblNodes->setText(QString("%1 đỉnh").arg(count)); lblEdges->setText(QString("%1 cạnh").arg(edges));
    lblStatus->setText(count == 0 ? "Chưa có đỉnh" : simulator->isFinished ? "Hoàn tất" : playTimer->isActive() ? "Đang chạy" : simulator->history.empty() ? "Sẵn sàng" : "Tạm dừng");
    QString route = "Chưa có đỉnh";
    if(count > 0 && simulator->targetNode < 0) {
        route = QString("%1 → mọi đỉnh\nDijkstra không có điểm kết thúc").arg(QString::fromStdString(simulator->graph[simulator->startNode].name));
        if (simulator->isFinished) {
            const int reachable=static_cast<int>(std::count(simulator->visited.begin(),simulator->visited.end(),true));
            route+=QString("\nĐã tính %1 đỉnh: %2 tới được, %3 không có đường (∞)").arg(count).arg(reachable).arg(count-reachable);
        }
    } else if(count > 0) {
        route = QString("%1 → %2").arg(QString::fromStdString(simulator->graph[simulator->startNode].name),QString::fromStdString(simulator->graph[simulator->targetNode].name));
        if(simulator->isFinished && simulator->dist[simulator->targetNode]!=std::numeric_limits<int>::max()) { QStringList nodes; for(int v=simulator->targetNode;v!=-1;v=simulator->prev[v]) nodes.prepend(QString::fromStdString(simulator->graph[v].name)); route = nodes.join(" → ")+QString("\nTổng trọng số: %1").arg(simulator->dist[simulator->targetNode]); }
        else route += simulator->isFinished ? "\nKhông tìm thấy đường đi" : "\nĐang tìm đường đi ngắn nhất";
    }
    lblPath->setText(route); btnPlay->setText(playTimer->isActive()?"Tạm dừng":"Chạy tự động"); btnPlay->setEnabled(!simulator->isFinished); btnNext->setEnabled(!simulator->isFinished); btnPrev->setEnabled(!simulator->history.empty()); updateCodeHighlight();
    if(pathPreview->isVisible()) showPathPreview(hoveredVertex,hoverPosition);
}

void MainWindow::hidePathPreview() {
    hoveredVertex=-1;
    pathPreview->hide();
}

void MainWindow::showPathPreview(int row,QPoint globalPosition) {
    if(simulator->targetNode>=0 || row<0 || row>=static_cast<int>(simulator->graph.size())) {
        hidePathPreview();return;
    }
    hoveredVertex=row;hoverPosition=globalPosition;pathPreview->setVertex(row);
    QScreen *screen=QGuiApplication::screenAt(globalPosition);
    if(!screen) screen=QGuiApplication::primaryScreen();
    QPoint position=globalPosition+QPoint(20,20);
    if(screen) {
        const QRect area=screen->availableGeometry().adjusted(8,8,-8,-8);
        if(position.x()+pathPreview->width()>area.right()) position.setX(globalPosition.x()-pathPreview->width()-20);
        position.setX(std::clamp(position.x(),area.left(),std::max(area.left(),area.right()-pathPreview->width())));
        position.setY(std::clamp(position.y(),area.top(),std::max(area.top(),area.bottom()-pathPreview->height())));
    }
    pathPreview->move(position);pathPreview->show();pathPreview->raise();
}
bool MainWindow::eventFilter(QObject *object,QEvent *event) {
    if(object==distTable->viewport()) {
        if(event->type()==QEvent::MouseMove) {
            const auto *mouse=static_cast<QMouseEvent*>(event);
            showPathPreview(distTable->indexAt(mouse->position().toPoint()).row(),mouse->globalPosition().toPoint());
        } else if(event->type()==QEvent::Leave || event->type()==QEvent::Hide || event->type()==QEvent::Wheel) {
            hidePathPreview();
        } else if(event->type()==QEvent::ToolTip) return true;
    }
    if(object==this && (event->type()==QEvent::WindowDeactivate || event->type()==QEvent::Hide)) {
        hidePathPreview();
    }
    return QMainWindow::eventFilter(object,event);
}
void MainWindow::updateCodeHighlight() {
    QList<QTextEdit::ExtraSelection> selections; auto block=codeViewer->document()->findBlockByLineNumber(simulator->currentCodeLine-1);
    if(block.isValid()) { QTextEdit::ExtraSelection selection; selection.cursor=QTextCursor(block); selection.cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor); selection.format.setBackground(QColor("#1d4ed8")); selection.format.setForeground(QColor("#ffffff")); selection.format.setFontWeight(QFont::Bold); selection.format.setProperty(QTextFormat::FullWidthSelection,true); selections.append(selection); codeViewer->setTextCursor(selection.cursor); codeViewer->ensureCursorVisible(); }
    codeViewer->setExtraSelections(selections);
}




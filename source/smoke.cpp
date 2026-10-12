#include "MainWindow.h"
#include "PathPreview.h"
#include <QApplication>
#include <QScreen>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QDir>
#include <QFontDatabase>
#include <QScrollBar>
#include <QScrollArea>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QInputDialog>
#include <QInputMethodEvent>
#include <QFileDialog>
#include <QFile>
#include <QSlider>
#include <QWheelEvent>
#include <QElapsedTimer>
#include <QEventLoop>
#include <random>
#include <cctype>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <algorithm>
int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
#ifdef Q_OS_WIN
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/consola.ttf");
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/seguisym.ttf");
#endif
    app.setFont(QFont("Segoe UI",10));
    const auto logicalPixel = [](const QImage &image, int x, int y) {
        return image.pixelColor(qRound(x*image.devicePixelRatio()), qRound(y*image.devicePixelRatio()));
    };
    DijkstraSimulator sim;
    std::string importError;
    assert(sim.importText("Circle\na b 1\nb c 2\n", importError));
    assert(sim.graph.size() == 3 && sim.graph[0].x == 300 && sim.graph[0].y == 50);
    assert(sim.importText("Square\na b 1\nb c 2\n", importError));
    assert(sim.graph[0].x == 50 && sim.graph[0].y == 50);
    assert(sim.importText("Freedom\na b 1\nb c 2\n", importError));
    assert(sim.graph[0].x != 50 || sim.graph[0].y != 50);
    assert(sim.importText("Grip\na b 1\nb c 2\n", importError));
    assert(sim.graph[0].x == 50 && sim.graph[0].y == 50);
    assert(sim.importText("UnknownLayout\na b 1\nb c 2\n", importError) == false);
    assert(sim.importText("a b 10\nc d 20\na c 10\n", importError));
    assert(sim.graph.size() == 4 && sim.targetNode == -1);
    sim.runToEnd();
    assert((sim.dist == std::vector<int>{0,10,10,30}) && sim.shortestPathEdges.size() == 3);
    assert(std::all_of(sim.visited.begin(), sim.visited.end(), [](bool v) { return v; }));
    sim.stepBack(); assert(!sim.isFinished); sim.step(); assert(sim.isFinished);
    assert(!sim.importText("a b -1", importError) && sim.graph.size() == 4);
    assert(!sim.importText("a b 1 extra", importError));
    assert(!sim.importText("a a 1", importError));
    assert(!sim.importText("a b 1.5", importError));
    assert(!sim.importText("\n", importError));
    std::string thirty;
    for (int i=1; i<30; ++i) thirty += "v0 v" + std::to_string(i) + " 1\n";
    assert(sim.importText(thirty, importError) && sim.graph.size() == 30);
    assert(!sim.importText(thirty + "v0 v30 2\n", importError) && sim.graph.size() == 30);
    assert(importError.find("30") != std::string::npos);
    assert(sim.importText("a b 2\nx y 3\na b 7\n", importError));
    sim.runToEnd();
    assert(sim.dist[1] == 7 && sim.dist[2] == std::numeric_limits<int>::max() && !sim.visited[2]);
    assert(sim.shortestPathEdges.size() == 1);
    assert(sim.removeNode(3) && sim.targetNode == -1);
    DijkstraSimulator arrowImport;
    for (auto type : {DijkstraSimulator::GraphType::Undirected, DijkstraSimulator::GraphType::Directed, DijkstraSimulator::GraphType::Mixed}) {
        DijkstraSimulator plainImport;plainImport.graphType=type;
        assert(plainImport.importText("# test\nCircle\nA   B\t3\nB -> C 5\nC -> B 8",importError));
        assert(plainImport.graphType==DijkstraSimulator::GraphType::Mixed);
        assert(!plainImport.graph[0].edges[0].directed && plainImport.graph[0].edges[0].weight==3);
        assert(!plainImport.graph[1].edges[0].directed && plainImport.graph[1].edges[0].target==0);
        plainImport.runToEnd();assert((plainImport.dist==std::vector<int>{0,3,8}));
        plainImport.startNode=2;plainImport.reset();plainImport.runToEnd();
        assert((plainImport.dist==std::vector<int>{11,8,0}));
    }
    assert(arrowImport.importText("a -> b 10\nb -> a 20\nb -- c 0\nx -> y 7",importError));
    assert(arrowImport.graphType==DijkstraSimulator::GraphType::Mixed && arrowImport.targetNode==-1);
    arrowImport.runToEnd();assert((arrowImport.dist==std::vector<int>{0,10,10,std::numeric_limits<int>::max(),std::numeric_limits<int>::max()}));
    assert((arrowImport.pathTo(2)==std::vector<int>{0,1,2}) && arrowImport.pathTo(3).empty() && (arrowImport.pathTo(0)==std::vector<int>{0}));
    assert(arrowImport.graph[1].edges[0].weight==20);
    arrowImport.startNode=1;arrowImport.reset();arrowImport.runToEnd();assert(arrowImport.dist[0]==20);
    assert(!arrowImport.importText("a -> b -1",importError) && arrowImport.graph.size()==5);
    assert(!arrowImport.importText("a -> b 10 extra",importError));
    assert(!arrowImport.importText("a -> a 10",importError));
    assert(!arrowImport.importText("a -> b 1.5",importError));
    // At any density the generated directed spanning tree reaches all vertices from source 0.
    for(int seed=0;seed<24;++seed) for(int density:{0,20,100}) for(int count:{2,13,30}) {
        DijkstraSimulator generated;generated.graphType=DijkstraSimulator::GraphType::Directed;
        assert(generated.generateRandomGraph(count,DijkstraSimulator::Layout::Circle,density,20,seed));
        generated.targetNode=-1;generated.reset();generated.runToEnd();
        assert(std::all_of(generated.visited.begin(),generated.visited.end(),[](bool v){return v;}));
        assert(generated.shortestPathEdges.size()==static_cast<size_t>(count-1));
        for(int v=0;v<count;++v) {const auto path=generated.pathTo(v);assert(!path.empty() && path.front()==0 && path.back()==v);}
    }
    // Render a long wrapped route as well as the source and unreachable states.
    DijkstraSimulator routeSim;
    for(int i=0;i<30;++i) routeSim.addNode(i*60,0);
    for(int i=0;i<29;++i) routeSim.setEdge(i,i+1,i%4,true);
    routeSim.targetNode=-1;routeSim.reset();routeSim.runToEnd();
    PathPreview routePreview(&routeSim);routePreview.setVertex(29);routePreview.show();app.processEvents();
    QDir().mkpath("checks");routePreview.grab().save("checks/preview-long-route.png");
    assert(routeSim.pathTo(29).size()==30 && routePreview.height()<700);
    routePreview.setVertex(0);routePreview.grab().save("checks/preview-source-route.png");
    routeSim.removeEdge(0,1);routeSim.runToEnd();routePreview.setVertex(29);
    assert(routeSim.pathTo(29).empty());routePreview.grab().save("checks/preview-unreachable-route.png");routePreview.hide();
    // Compare directed and mixed graphs with an independent Bellman-Ford oracle.
    for(int mode=0;mode<3;++mode) for(int seed=0;seed<12;++seed) {
        DijkstraSimulator test;std::mt19937 rng(seed);
        for(int i=0;i<12;++i) assert(test.addNode(i*60,0));
        for(int a=0;a<12;++a) for(int b=a+1;b<12;++b) if(rng()%4==0) {
            const bool directed=mode==1 || (mode==2 && rng()%2);
            assert(test.setEdge(a,b,rng()%20,directed));
            if(directed && rng()%2) assert(test.setEdge(b,a,rng()%20,true));
        }
        for(int source=0;source<12;++source) {
            test.startNode=source;test.targetNode=-1;test.reset();
            std::vector<int> ref(12,std::numeric_limits<int>::max());ref[source]=0;
            for(int pass=0;pass<11;++pass) for(const auto &n:test.graph) if(ref[n.id]!=std::numeric_limits<int>::max())
                for(const auto &e:n.edges) ref[e.target]=std::min(ref[e.target],ref[n.id]+e.weight);
            test.runToEnd();assert(test.dist==ref);
            for(int target=0;target<12;++target) {
                test.targetNode=target;test.reset();test.runToEnd();assert(test.dist[target]==ref[target]);
                for(const auto &[from,to]:test.shortestPathEdges)
                    assert(std::any_of(test.graph[from].edges.begin(),test.graph[from].edges.end(),[&](const Edge &e){return e.target==to;}));
            }
        }
    }
    DijkstraSimulator directed;
    assert(directed.importText("a b 2 ->\nb a 9 ->\nb c 0 --\nx y 7 ->",importError));
    directed.runToEnd();assert((directed.dist==std::vector<int>{0,2,2,std::numeric_limits<int>::max(),std::numeric_limits<int>::max()}));
    assert(directed.setEdge(0,1,5,true) && directed.graph[1].edges[0].weight==9);
    assert(directed.removeEdge(0,1) && directed.graph[1].edges[0].target==0);
    assert(directed.setEdge(0,1,3,false));assert(!directed.graph[1].edges[0].directed && directed.graph[1].edges[0].weight==3);
    assert(directed.setEdge(0,1,4,true));assert(std::none_of(directed.graph[1].edges.begin(),directed.graph[1].edges.end(),[](const Edge &e){return e.target==0;}));
    directed.targetNode=1;assert(directed.removeNode(1) && directed.targetNode==-1);
    directed.graphType=DijkstraSimulator::GraphType::Directed;assert(directed.importText("a b 3",importError));
    assert(!directed.graph[0].edges[0].directed && directed.graph[1].edges[0].weight==3);
    directed.setGraphType(DijkstraSimulator::GraphType::Directed);
    assert(directed.generateRandomGraph(30,DijkstraSimulator::Layout::Grid,100,9999,42));
    directed.targetNode=-1;directed.runToEnd();assert(directed.dist[29]!=std::numeric_limits<int>::max());
    std::cout << "Directed/mixed/all-source and single-target oracle checks passed\n";
    DijkstraSimulator modes;modes.loadTemplate(0);modes.step();
    const int oldX=modes.graph[0].x;
    modes.setGraphType(DijkstraSimulator::GraphType::Directed);
    assert(modes.history.empty() && modes.graph[0].x==oldX);
    int modeEdges=0;
    for(const auto &n:modes.graph) for(const auto &e:n.edges) {assert(e.directed && n.id<e.target);++modeEdges;}
    assert(modeEdges==8);
    modes.setGraphType(DijkstraSimulator::GraphType::Mixed);
    assert(modes.newEdgesDirected());assert(modes.setEdge(1,0,9,true));
    modes.setGraphType(DijkstraSimulator::GraphType::Undirected);
    assert(!modes.newEdgesDirected());
    for(const auto &n:modes.graph) for(const auto &e:n.edges) {
        assert(!e.directed);
        assert(std::any_of(modes.graph[e.target].edges.begin(),modes.graph[e.target].edges.end(),[&](const Edge &r){return r.target==n.id && r.weight==e.weight && !r.directed;}));
    }
    assert(modes.graph[0].edges[0].weight==4);
    assert(modes.graph[0].edges.size()==3); // A-B weights 4 and 9 remain separate.
    assert(std::count_if(modes.graph[0].edges.begin(),modes.graph[0].edges.end(),[](const Edge &e){return e.target==1 && e.weight==9;})==1);
    DijkstraSimulator unmerge;
    unmerge.addNode(100,250);unmerge.addNode(600,250);unmerge.targetNode=-1;
    unmerge.addEdge(0,1,8,true);unmerge.addEdge(0,1,3,true);
    unmerge.addEdge(1,0,6,true);unmerge.addEdge(0,1,10,false);
    std::vector<int> originalIds;
    for(const auto &e:unmerge.graph[0].edges) originalIds.push_back(e.id);
    originalIds.push_back(unmerge.graph[1].edges[0].id);
    unmerge.setGraphType(DijkstraSimulator::GraphType::Undirected);
    for(int repeat=0;repeat<3;++repeat) {
        assert(unmerge.graph[0].edges.size()==4 && unmerge.graph[1].edges.size()==4);
        for(int id:originalIds) for(int vertex:{0,1})
            assert(std::count_if(unmerge.graph[vertex].edges.begin(),unmerge.graph[vertex].edges.end(),[id](const Edge &e){return e.id==id && !e.directed;})==1);
        unmerge.startNode=repeat%2;unmerge.reset();unmerge.runToEnd();
        assert(unmerge.dist[1-unmerge.startNode]==3);
        unmerge.setGraphType(DijkstraSimulator::GraphType::Undirected);
    }
    assert(unmerge.editEdge(0,originalIds[0],12,false));
    assert(unmerge.graph[0].edges[1].weight==3 && unmerge.graph[1].edges[0].weight==6);
    assert(unmerge.removeEdgeById(originalIds[1]));
    assert(unmerge.graph[0].edges.size()==3 && unmerge.graph[1].edges.size()==3);
    unmerge.setGraphType(DijkstraSimulator::GraphType::Directed);
    unmerge.setGraphType(DijkstraSimulator::GraphType::Undirected);
    assert(unmerge.graph[0].edges.size()==3 && unmerge.graph[1].edges.size()==3);
    modes.setGraphType(DijkstraSimulator::GraphType::Mixed);
    modes.setGraphType(DijkstraSimulator::GraphType::Mixed);
    assert(modes.importText("a b 3",importError) && modes.graphType==DijkstraSimulator::GraphType::Mixed && !modes.graph[0].edges[0].directed);
    assert(modes.graph[1].edges[0].target==0 && modes.graph[1].edges[0].weight==3);
    modes.setGraphType(DijkstraSimulator::GraphType::Directed);
    modes.loadTemplate(1);
    assert(modes.graphType==DijkstraSimulator::GraphType::Directed);
    for(const auto &n:modes.graph) for(const auto &e:n.edges) assert(e.directed);
    sim.loadTemplate(0);
    sim.step();
    assert(sim.setEdge(0, 5, 0));
    assert(sim.history.empty() && sim.currentCodeLine == 2);
    assert(sim.setEdge(0, 5, 7));
    int matching = 0;
    for (const auto &e : sim.graph[0].edges) if(e.target == 5) { ++matching; assert(e.weight == 7); }
    assert(matching == 1);
    for (const auto &e : sim.graph[5].edges) if(e.target == 0) assert(e.weight == 7);
    assert(!sim.setEdge(0, 0, 2) && !sim.setEdge(-1, 1, 2) && !sim.setEdge(0, 1, -1));
    assert(!sim.setEdge(0, 1, DijkstraSimulator::MaxEdgeWeight + 1));
    assert(sim.removeEdge(0, 5) && !sim.removeEdge(0, 5));
    assert(sim.removeEdge(3, 5) && sim.removeEdge(4, 5));
    for(int i=0; !sim.isFinished && i<500; ++i) sim.step();
    assert(sim.isFinished && sim.dist[5] == std::numeric_limits<int>::max() && sim.shortestPathEdges.empty());
    assert(sim.generateRandomGraph(2, DijkstraSimulator::Layout::Circle, 0, 1, 42));
    assert(sim.setEdge(0, 1, 0));
    while(!sim.isFinished) sim.step();
    assert(sim.dist[1] == 0);
    assert(!sim.generateRandomGraph(1, DijkstraSimulator::Layout::Circle));
    assert(!sim.generateRandomGraph(31, DijkstraSimulator::Layout::Circle));
    assert(!sim.generateRandomGraph(4, DijkstraSimulator::Layout::Circle, 101));
    assert(!sim.generateRandomGraph(4, DijkstraSimulator::Layout::Circle, 10, 0));
    for (auto style : {DijkstraSimulator::Layout::Circle, DijkstraSimulator::Layout::Square,
                       DijkstraSimulator::Layout::Grid, DijkstraSimulator::Layout::Random}) {
        for(int count : {2, 8, 30}) for(int density : {0, 20, 100}) {
            assert(sim.generateRandomGraph(count, style, density, 9999, 42));
            int edgeCount = 0;
            for (const auto &node : sim.graph) {
                assert(!node.name.empty() && std::all_of(node.name.begin(), node.name.end(), [](unsigned char c) { return std::isupper(c); }));
                std::vector<bool> seen(count, false);
                for (const auto &edge : node.edges) {
                    assert(edge.target != node.id && !seen[edge.target]); seen[edge.target] = true;
                    assert(edge.weight >= 1 && edge.weight <= 9999);
                    const auto &reverse = sim.graph[edge.target].edges;
                    assert(std::any_of(reverse.begin(), reverse.end(), [&](const Edge &e) { return e.target == node.id && e.weight == edge.weight; }));
                    ++edgeCount;
                }
            }
            if (density == 0) assert(edgeCount == 2 * (count - 1));
            if (density == 100) assert(edgeCount == count * (count - 1));
            // Independent Bellman-Ford reference also verifies connectivity.
            std::vector<int> reference(count, std::numeric_limits<int>::max()); reference[0] = 0;
            for(int pass=0; pass<count-1; ++pass) for(const auto &node : sim.graph) {
                if(reference[node.id] == std::numeric_limits<int>::max()) continue;
                for(const auto &edge : node.edges) reference[edge.target] = std::min(reference[edge.target], reference[node.id] + edge.weight);
            }
            for(int distance : reference) assert(distance != std::numeric_limits<int>::max());
            for(int i=0; !sim.isFinished && i<30000; ++i) sim.step();
            assert(sim.isFinished && sim.dist[count-1] == reference[count-1]);
            sim.targetNode = -1; sim.reset(); sim.runToEnd();
            assert(sim.dist == reference);
            assert(sim.shortestPathEdges.size() == static_cast<size_t>(count-1));
        }
    }
    const int expected[] = {13,10,8,6,9};
    for(int t=0;t<5;++t) {
        sim.loadTemplate(t);
        for (const auto &node : sim.graph)
            assert(node.name == std::string(1, static_cast<char>('A' + node.id)));
        assert(sim.pqState.empty());
        int steps=0;
        while(!sim.isFinished && steps++<500) {
            const auto oldDist=sim.dist;
            const auto oldPrev=sim.prev;
            const auto oldLine=sim.currentCodeLine;
            sim.step();
            if(oldDist!=sim.dist) assert(sim.currentCodeLine==11);
            sim.stepBack();
            assert(sim.dist==oldDist && sim.prev==oldPrev && sim.currentCodeLine==oldLine);
            sim.step();
        }
        assert(sim.isFinished && sim.dist[sim.targetNode]==expected[t]);
        sim.stepBack(); assert(!sim.isFinished); sim.step(); assert(sim.isFinished);
        std::cout << "Template " << t << ": " << steps << " steps, distance " << expected[t] << '\n';
    }

    sim.loadTemplate(0); sim.step();
    assert(sim.addNode(300, 200) && sim.history.empty());
    assert(sim.graph.back().name == "G");
    assert(sim.setEdge(6, 5, 12));
    sim.startNode = 6; sim.targetNode = 5;
    assert(sim.removeNode(2));
    assert(sim.startNode == 5 && sim.targetNode == 4);
    for(const auto &n : sim.graph) for(const auto &e : n.edges) {
        assert(e.target >= 0 && e.target < static_cast<int>(sim.graph.size()));
        assert(std::any_of(sim.graph[e.target].edges.begin(), sim.graph[e.target].edges.end(), [&](const Edge &back) { return back.target == n.id && back.weight == e.weight; }));
    }
    while(!sim.graph.empty()) assert(sim.removeNode(0));
    assert(sim.isFinished && sim.startNode == -1 && sim.dist.empty());
    sim.step(); assert(sim.history.empty());
    assert(sim.addNode(100,100) && sim.startNode == 0 && sim.targetNode == -1);
    while(!sim.isFinished) sim.step();
    assert(sim.dist[0] == 0);

    // Building on a blank canvas must keep all-destinations mode through edits.
    sim.loadTemplate(5);
    for (int i = 0; i < 4; ++i) assert(sim.addNode(100 + i * 100, 100));
    assert(sim.startNode == 0 && sim.targetNode == -1);
    assert(sim.setEdge(0, 1, 2) && sim.setEdge(1, 2, 3) && sim.setEdge(0, 2, 9));
    assert(sim.renameNode(1, "Middle") && sim.targetNode == -1);
    int examinedEdges = 0;
    for (int steps = 0; !sim.isFinished && steps < 500; ++steps) {
        if (sim.currentCodeLine == 10) ++examinedEdges;
        sim.step();
    }
    assert(sim.isFinished && examinedEdges == 6);
    assert((sim.dist == std::vector<int>{0, 2, 5, std::numeric_limits<int>::max()}));
    assert(sim.visited[0] && sim.visited[1] && sim.visited[2] && !sim.visited[3]);
    assert(sim.shortestPathEdges.size() == 2);
    sim.reset(); sim.runToEnd();
    assert(sim.dist[2] == 5 && sim.targetNode == -1);
    sim.targetNode = 1; sim.reset(); sim.runToEnd();
    assert(sim.visited[1] && !sim.visited[2]);

    // Drive the actual mouse events and popup controls against a standalone graph.
    sim.loadTemplate(0);
    GraphWidget interactive(&sim); interactive.resize(800,500); interactive.show(); app.processEvents();
    auto popup = [&](QWidget *widget, QPoint position, auto check) {
        widget->repaint();app.processEvents();
        QTimer::singleShot(0, [&, check] {
            auto *menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()); assert(menu && menu->objectName() == "t1");
            check(menu); menu->close();
        });
        QContextMenuEvent event(QContextMenuEvent::Mouse,position,widget->mapToGlobal(position));
        QApplication::sendEvent(widget,&event); app.processEvents();
    };
    QMouseEvent editWeight(QEvent::MouseButtonDblClick,QPointF(188,159),interactive.mapToGlobal(QPoint(188,159)),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(&interactive,&editWeight);app.processEvents();
    auto *inlineWeight=interactive.findChild<QSpinBox*>("inlineEdgeWeight");
    assert(inlineWeight && inlineWeight->isVisible());
    interactive.grab().save("checks/preview-inline-weight.png");
    assert(!inlineWeight->hasFrame() && inlineWeight->styleSheet().contains("background: transparent"));
    inlineWeight->setValue(8);
    QKeyEvent weightConfirm(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
    QApplication::sendEvent(inlineWeight,&weightConfirm);
    assert(sim.graph[0].edges[0].weight==8 && sim.graph[1].edges[0].weight==8);
    assert(sim.setEdge(0,1,4));app.processEvents();
    QApplication::sendEvent(&interactive,&editWeight);app.processEvents();
    inlineWeight->setValue(11);
    QKeyEvent weightCancel(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
    QApplication::sendEvent(inlineWeight,&weightCancel);
    assert(sim.graph[0].edges[0].weight==4 && !inlineWeight->isVisible());
    QDir().mkpath("checks");
    popup(&interactive,QPoint(188,159),[&](QMenu *menu) {
        auto *input = menu->findChild<QSpinBox*>("weightInput");
        auto *plus = menu->findChild<QPushButton*>("weightPlus");
        auto *minus = menu->findChild<QPushButton*>("weightMinus");
        assert(input && plus && minus && input->value() == 4);
        plus->click(); assert(sim.graph[0].edges[0].weight == 5);
        minus->click(); assert(sim.graph[1].edges[0].weight == 4);
        input->setValue(0); assert(!minus->isEnabled() && sim.graph[0].edges[0].weight == 0);
        input->setValue(9999); assert(!plus->isEnabled());
        input->findChild<QLineEdit*>()->setText("17");
        QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(input, &enter);
        assert(input->value() == 17 && sim.graph[0].edges[0].weight == 17 && sim.graph[1].edges[0].weight == 17);
        menu->grab().save("checks/preview-weight-menu.png");
    });
    popup(&interactive,QPoint(97,250),[&](QMenu *menu) {
        assert(menu->actions().size() == 3);
        assert(menu->actions()[0]->text() == "Đặt làm điểm xuất phát");
        assert(menu->actions()[1]->text() == "Đặt làm điểm kết thúc");
        menu->grab().save("checks/preview-node-menu.png");
        menu->actions()[0]->trigger(); assert(sim.startNode == 0);
        menu->actions()[1]->trigger(); assert(sim.targetNode == 0);
    });
    popup(&interactive, QPoint(97,250), [&](QMenu *menu) {
        assert(!menu->findChild<QAction*>("renameVertex"));
        auto *clear = menu->findChild<QAction*>("clearTarget"); assert(clear);
        clear->trigger(); assert(sim.targetNode == -1);
    });
    auto doubleClick = [&](QPoint point) {
        QMouseEvent event(QEvent::MouseButtonDblClick, QPointF(point), interactive.mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&interactive, &event); app.processEvents();
    };
    doubleClick(QPoint(97,250));
    auto *inlineName = interactive.findChild<QLineEdit*>("inlineVertexName");
    assert(inlineName && inlineName->isVisible() && !QApplication::activeModalWidget());
    inlineName->selectAll(); inlineName->insert("iiii");
    assert(inlineName->text() == "iiii" && inlineName->hasAcceptableInput());
    inlineName->insert("i"); assert(inlineName->text() == "iiii");
    inlineName->selectAll(); inlineName->insert("WWWW");
    assert(inlineName->text() == "iiii"); // Four wide letters do not fit.
    inlineName->selectAll();
    QKeyEvent wideLetter(QEvent::KeyPress, Qt::Key_W, Qt::NoModifier, "W");
    QApplication::sendEvent(inlineName, &wideLetter);
    assert(inlineName->text() == "W");
    QApplication::sendEvent(inlineName, &wideLetter);
    assert(inlineName->text() == "WW");
    QApplication::sendEvent(inlineName, &wideLetter);
    assert(inlineName->text() == "WW"); // Block at the boundary, before four characters.
    QKeyEvent eraseLetter(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    QApplication::sendEvent(inlineName, &eraseLetter);
    QApplication::sendEvent(inlineName, &wideLetter);
    assert(inlineName->text() == "WW"); // Editing still works after a rejected key.
    const QVariant cursorBeforePreedit = inlineName->inputMethodQuery(Qt::ImCursorRectangle);
    QInputMethodEvent overflowingPreedit("W", {});
    QApplication::sendEvent(inlineName, &overflowingPreedit);
    const QVariant preeditCursor = inlineName->inputMethodQuery(Qt::ImCursorPosition);
    assert(inlineName->text() == "WW" && preeditCursor.toInt() == 2);
    assert(inlineName->inputMethodQuery(Qt::ImCursorRectangle) == cursorBeforePreedit);
    QInputMethodEvent overflowingCommit;
    overflowingCommit.setCommitString("W");
    QApplication::sendEvent(inlineName, &overflowingCommit);
    assert(inlineName->text() == "WW");
    inlineName->setText(QString::fromUtf8("a\u0301"));
    assert(inlineName->hasAcceptableInput()); // Combining accent is one character.
    inlineName->setText("iiii"); app.processEvents();
    interactive.grab().save("checks/preview-inline-rename.png");
    QKeyEvent confirmName(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(inlineName, &confirmName);
    assert(sim.graph[0].name == "iiii" && sim.history.empty() && !inlineName->isVisible());
    doubleClick(QPoint(97,250)); inlineName->setText("B");
    QApplication::sendEvent(inlineName, &confirmName); assert(sim.graph[0].name == "iiii");
    doubleClick(QPoint(97,250)); inlineName->setText("12345");
    QApplication::sendEvent(inlineName, &confirmName); assert(sim.graph[0].name == "iiii");
    doubleClick(QPoint(97,250)); inlineName->setText("Canceled");
    QKeyEvent cancelName(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(inlineName, &cancelName); assert(sim.graph[0].name == "iiii");
    assert(!sim.renameNode(0, "B") && !sim.renameNode(0, "") && !sim.renameNode(-1, "X"));
    assert(sim.renameNode(0, "A"));
    // Selection survives a drag; blank clicks clear it; Backspace deletes only a selection.
    auto leftClick = [&](QPoint point) {
        QMouseEvent down(QEvent::MouseButtonPress, QPointF(point), interactive.mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent up(QEvent::MouseButtonRelease, QPointF(point), interactive.mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&interactive, &down); QApplication::sendEvent(&interactive, &up); app.processEvents();
    };
    leftClick(QPoint(97,250));
    const QImage selectedImage = interactive.grab().toImage();
    assert(logicalPixel(selectedImage,130,250).red() > 200 && logicalPixel(selectedImage,130,250).green() < 150);
    interactive.grab().save("checks/preview-selected.png");
    leftClick(QPoint(5,5));
    QKeyEvent backspace(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    QApplication::sendEvent(&interactive, &backspace); assert(sim.graph.size() == 6);
    popup(&interactive, QPoint(188,159), [&](QMenu *menu) {
        auto *remove = menu->findChild<QAction*>("deleteEdge"); assert(remove);
        // An uncommitted edit must never recreate the edge after deletion.
        menu->findChild<QSpinBox*>("weightInput")->findChild<QLineEdit*>()->setText("23");
        menu->grab().save("checks/preview-weight-menu.png");
        remove->trigger();
    });
    assert(std::none_of(sim.graph[0].edges.begin(), sim.graph[0].edges.end(), [](const Edge &e) { return e.target == 1; }));
    assert(std::none_of(sim.graph[1].edges.begin(), sim.graph[1].edges.end(), [](const Edge &e) { return e.target == 0; }));
    auto rightDrag = [&](QPoint from, QPoint to, bool savePreview = false) {
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(from),interactive.mapToGlobal(from),Qt::RightButton,Qt::RightButton,Qt::NoModifier);
        QApplication::sendEvent(&interactive,&press);
        QContextMenuEvent early(QContextMenuEvent::Mouse,from,interactive.mapToGlobal(from));
        QApplication::sendEvent(&interactive,&early); // Windows may request its menu on press.
        const QImage before = interactive.grab().toImage();
        QMouseEvent move(QEvent::MouseMove,QPointF(to),interactive.mapToGlobal(to),Qt::NoButton,Qt::RightButton,Qt::NoModifier);
        QApplication::sendEvent(&interactive,&move); app.processEvents();
        if (savePreview) {
            assert(interactive.grab().toImage() != before);
            interactive.grab().save("checks/preview-connect-drag.png");
        }
        QMouseEvent release(QEvent::MouseButtonRelease,QPointF(to),interactive.mapToGlobal(to),Qt::RightButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(&interactive,&release);
        QContextMenuEvent late(QContextMenuEvent::Mouse,to,interactive.mapToGlobal(to));
        QApplication::sendEvent(&interactive,&late); // Suppress the release menu after a drag.
        app.processEvents(); assert(!QApplication::activePopupWidget());
    };
    rightDrag(QPoint(97,250), QPoint(278,68), true);
    assert(sim.graph[0].edges.back().target == 1 && sim.graph[0].edges.back().weight == 1);
    assert(sim.setEdge(0,1,9));
    const auto edgeCount = sim.graph[0].edges.size();
    rightDrag(QPoint(97,250), QPoint(278,68));
    assert(sim.graph[0].edges.size() == edgeCount + 1 && sim.graph[0].edges.back().weight == 1);
    assert(sim.graph[0].edges[edgeCount-1].weight == 9);
    assert(sim.removeEdgeById(sim.graph[0].edges.back().id));
    rightDrag(QPoint(97,250), QPoint(20,20));
    rightDrag(QPoint(97,250), QPoint(120,250)); // Releasing on the source must not create a self-loop.
    assert(sim.graph[0].edges.size() == edgeCount);
    // A normal right-click still opens the node menu, without creating an edge.
    QMouseEvent rightPress(QEvent::MouseButtonPress,QPointF(97,250),interactive.mapToGlobal(QPoint(97,250)),Qt::RightButton,Qt::RightButton,Qt::NoModifier);
    QApplication::sendEvent(&interactive,&rightPress);
    QTimer::singleShot(0, [&] {
        auto *menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()); assert(menu);
        assert(menu->findChild<QAction*>("clearTarget") && !menu->findChild<QAction*>("renameVertex")); menu->close();
    });
    QMouseEvent rightRelease(QEvent::MouseButtonRelease,QPointF(97,250),interactive.mapToGlobal(QPoint(97,250)),Qt::RightButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(&interactive,&rightRelease);
    QContextMenuEvent rightContext(QContextMenuEvent::Mouse,QPoint(97,250),interactive.mapToGlobal(QPoint(97,250)));
    QApplication::sendEvent(&interactive,&rightContext);
    const QImage beforeDrag=interactive.grab().toImage();
    QMouseEvent press(QEvent::MouseButtonPress,QPointF(97,250),interactive.mapToGlobal(QPoint(97,250)),Qt::LeftButton,Qt::LeftButton,Qt::AltModifier);
    QApplication::sendEvent(&interactive,&press);
    QMouseEvent move(QEvent::MouseMove,QPointF(150,310),interactive.mapToGlobal(QPoint(150,310)),Qt::NoButton,Qt::LeftButton,Qt::AltModifier);
    QApplication::sendEvent(&interactive,&move);
    QMouseEvent release(QEvent::MouseButtonRelease,QPointF(150,310),interactive.mapToGlobal(QPoint(150,310)),Qt::LeftButton,Qt::NoButton,Qt::AltModifier);
    QApplication::sendEvent(&interactive,&release); app.processEvents();
    assert(sim.graph[0].x == 150 && sim.graph[0].y == 310);
    const auto movedImage=interactive.grab().toImage();
    assert(logicalPixel(movedImage,183,310).red() > 200 && logicalPixel(movedImage,183,310).green() < 150);
    assert(interactive.grab().toImage() != beforeDrag);
    popup(&interactive,QPoint(150,310),[&](QMenu *menu) {
        assert(!menu->findChild<QSpinBox*>()); menu->actions()[1]->trigger(); assert(sim.targetNode == 0);
    });
    interactive.grab().save("checks/preview-dragged.png");
    popup(&interactive,QPoint(400,250),[&](QMenu *menu) {
        assert(menu->actions().size() == 1);
        assert(menu->actions()[0]->text() == "+  Thêm đỉnh tại đây");
        menu->grab().save("checks/preview-canvas-menu.png");
        menu->actions()[0]->trigger(); assert(sim.graph.size() == 7);
        assert(sim.graph.back().x == 400 && sim.graph.back().y == 250);
    });
    rightDrag(QPoint(400,250), QPoint(150,310), true);
    assert(sim.graph[6].edges.size() == 1 && sim.graph[6].edges[0].target == 0);
    assert(sim.graph[0].edges.back().target == 6);
    popup(&interactive,QPoint(5,5),[&](QMenu *menu) {
        assert(menu->actions().size() == 1);
    });
    leftClick(QPoint(400,250));
    QApplication::sendEvent(&interactive, &backspace); assert(sim.graph.size() == 6);
    interactive.close();
    DijkstraSimulator selectionSim;selectionSim.loadTemplate(0);
    GraphWidget selection(&selectionSim);selection.resize(800,500);selection.show();app.processEvents();
    auto sendMouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons,Qt::KeyboardModifiers mods=Qt::NoModifier){
        QMouseEvent event(type,point,selection.mapToGlobal(point.toPoint()),button,buttons,mods);QApplication::sendEvent(&selection,&event);app.processEvents();
    };
    const QImage beforeZoom=selection.grab().toImage();
    QWheelEvent wheel(QPointF(350,250),selection.mapToGlobal(QPoint(350,250)),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    QApplication::sendEvent(&selection,&wheel);app.processEvents();assert(selection.zoomFactor()>1 && selection.grab().toImage()!=beforeZoom);
    selection.resetView();app.processEvents();assert(selection.zoomFactor()==1);
    sendMouse(QEvent::MouseButtonPress,{97,250},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{97,250},Qt::LeftButton,Qt::NoButton);
    sendMouse(QEvent::MouseButtonPress,{278,68},Qt::LeftButton,Qt::LeftButton,Qt::ControlModifier);
    sendMouse(QEvent::MouseButtonRelease,{278,68},Qt::LeftButton,Qt::NoButton,Qt::ControlModifier);
    sendMouse(QEvent::MouseButtonPress,{97,250},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseMove,{107,260},Qt::NoButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{107,260},Qt::LeftButton,Qt::NoButton);
    assert(selectionSim.graph[0].x==107 && selectionSim.graph[1].x==289);
    selection.zoomBy(1.2);app.processEvents();
    sendMouse(QEvent::MouseButtonPress,{1,1},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseMove,{799,499},Qt::NoButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{799,499},Qt::LeftButton,Qt::NoButton);
    selection.grab().save("checks/preview-multi-selection.png");
    QApplication::sendEvent(&selection,&backspace);assert(selectionSim.graph.empty());
    selection.close();
    // Selecting only an edge deletes both arcs of an undirected edge, keeping nodes.
    selectionSim.loadTemplate(0);selection.resetLayout();selection.resetView();selection.show();app.processEvents();
    sendMouse(QEvent::MouseButtonPress,{188,159},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{188,159},Qt::LeftButton,Qt::NoButton);
    QApplication::sendEvent(&selection,&backspace);assert(selectionSim.graph.size()==6);
    assert(std::none_of(selectionSim.graph[0].edges.begin(),selectionSim.graph[0].edges.end(),[](const Edge &e){return e.target==1;}));
    selection.close();
    // Reverse directed arcs must retain independent weights after inline editing.
    selectionSim.graph.clear();selectionSim.addNode(100,250);selectionSim.addNode(600,250);
    assert(selectionSim.setEdge(0,1,3,true) && selectionSim.setEdge(1,0,9,true));
    selection.resetLayout();selection.show();app.processEvents();
    selection.grab().save("checks/preview-directed.png");
    QMouseEvent directedEdit(QEvent::MouseButtonDblClick,QPointF(400,262),selection.mapToGlobal(QPoint(400,262)),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(&selection,&directedEdit);app.processEvents();
    auto *directedWeight=selection.findChild<QSpinBox*>("inlineEdgeWeight");assert(directedWeight->isVisible() && directedWeight->value()==3);
    directedWeight->setValue(5);QApplication::sendEvent(directedWeight,&weightConfirm);
    assert(selectionSim.graph[0].edges[0].weight==5 && selectionSim.graph[1].edges[0].weight==9);
    // Left dragging an already selected vertex creates an edge without moving it.
    selectionSim.setGraphType(DijkstraSimulator::GraphType::Undirected);
    while(selectionSim.removeEdge(0,1)) {} // Clear both preserved physical edges for this fixture.
    app.processEvents();
    sendMouse(QEvent::MouseButtonPress,{65,250},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{65,250},Qt::LeftButton,Qt::NoButton);
    const int sourceX=selectionSim.graph[0].x;
    sendMouse(QEvent::MouseButtonPress,{65,250},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseMove,{735,250},Qt::NoButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{735,250},Qt::LeftButton,Qt::NoButton);
    assert(selectionSim.graph[0].x==sourceX && selectionSim.graph[0].edges.size()==1 && !selectionSim.graph[0].edges[0].directed);
    sendMouse(QEvent::MouseButtonPress,{65,250},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseMove,{735,250},Qt::NoButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{735,250},Qt::LeftButton,Qt::NoButton);
    assert(selectionSim.graph[0].edges.size()==2 && selectionSim.graph[1].edges.size()==2);
    assert(selectionSim.graph[0].edges[0].id!=selectionSim.graph[0].edges[1].id);
    selectionSim.removeEdgeById(selectionSim.graph[0].edges[1].id);
    selectionSim.setGraphType(DijkstraSimulator::GraphType::Mixed);
    selectionSim.setEdge(0,1,7,false);app.processEvents();
    sendMouse(QEvent::MouseButtonPress,{65,250},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseMove,{735,250},Qt::NoButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{735,250},Qt::LeftButton,Qt::NoButton);
    assert(selectionSim.graph[0].edges.size()==1 && selectionSim.graph[0].edges[0].directed && selectionSim.graph[1].edges.empty());
    assert(selectionSim.graph[0].edges[0].weight==7);
    auto overlayRightDrag = [&](QPoint from, QPoint to) {
        sendMouse(QEvent::MouseButtonPress,from,Qt::RightButton,Qt::RightButton);
        sendMouse(QEvent::MouseMove,to,Qt::NoButton,Qt::RightButton);
        sendMouse(QEvent::MouseButtonRelease,to,Qt::RightButton,Qt::NoButton);
        QContextMenuEvent lateContext(QContextMenuEvent::Mouse,to,selection.mapToGlobal(to));
        QApplication::sendEvent(&selection,&lateContext);app.processEvents();
    };
    // Repeated connections create a new edge without changing the old weight.
    selectionSim.setGraphType(DijkstraSimulator::GraphType::Directed);
    overlayRightDrag({65,250},{735,250});
    assert(selectionSim.graph[0].edges.size()==2 && selectionSim.graph[0].edges[0].weight==7);
    assert(selectionSim.graph[0].edges[1].weight==1);
    assert(selectionSim.graph[0].edges[0].id!=selectionSim.graph[0].edges[1].id);
    selectionSim.removeEdgeById(selectionSim.graph[0].edges[1].id);
    // Overlaying an undirected edge in reverse uses the drag direction and keeps its weight.
    selectionSim.setGraphType(DijkstraSimulator::GraphType::Mixed);
    selectionSim.setEdge(0,1,11,false);app.processEvents();
    overlayRightDrag({735,250},{65,250});
    assert(selectionSim.graph[0].edges.empty() && selectionSim.graph[1].edges.size()==1);
    assert(selectionSim.graph[1].edges[0].directed && selectionSim.graph[1].edges[0].weight==11);
    // The opposite directed arc is independent of the existing arc.
    overlayRightDrag({65,250},{735,250});
    assert(selectionSim.graph[0].edges.size()==1 && selectionSim.graph[0].edges[0].weight==1);
    assert(selectionSim.graph[1].edges.size()==1 && selectionSim.graph[1].edges[0].weight==11);
    selectionSim.removeEdge(1,0);app.processEvents();
    popup(&selection,QPoint(400,250),[&](QMenu *menu){
        auto *removeDirection=menu->findChild<QAction*>("toggleEdgeDirection");assert(removeDirection && removeDirection->text()=="Xóa hướng");removeDirection->trigger();
    });
    assert(!selectionSim.graph[0].edges[0].directed && selectionSim.graph[1].edges.size()==1);
    selectionSim.setGraphType(DijkstraSimulator::GraphType::Undirected);app.processEvents();
    popup(&selection,QPoint(400,250),[&](QMenu *menu){assert(!menu->findChild<QAction*>("toggleEdgeDirection"));});
    selectionSim.setGraphType(DijkstraSimulator::GraphType::Directed);app.processEvents();
    // Right-click a segment away from its weight, without selecting first.
    sendMouse(QEvent::MouseButtonPress,{230,250},Qt::RightButton,Qt::RightButton);
    sendMouse(QEvent::MouseButtonRelease,{230,250},Qt::RightButton,Qt::NoButton);
    popup(&selection,QPoint(230,250),[&](QMenu *menu){
        auto *action=menu->findChild<QAction*>("toggleEdgeDirection");assert(action && action->text()=="Xóa hướng");action->trigger();
    });
    assert(selectionSim.graphType==DijkstraSimulator::GraphType::Mixed && !selectionSim.graph[0].edges[0].directed);
    assert(selectionSim.graph[1].edges[0].target==0 && selectionSim.graph[1].edges[0].weight==selectionSim.graph[0].edges[0].weight);
    // Select a directed edge, then right-click its line and remove direction.
    selectionSim.setEdge(0,1,7,true);app.processEvents();
    sendMouse(QEvent::MouseButtonPress,{230,250},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{230,250},Qt::LeftButton,Qt::NoButton);
    popup(&selection,QPoint(230,250),[&](QMenu *menu){
        auto *action=menu->findChild<QAction*>("toggleEdgeDirection");assert(action);menu->grab().save("checks/preview-remove-direction.png");action->trigger();
    });
    assert(!selectionSim.graph[0].edges[0].directed && selectionSim.graph[0].edges[0].weight==7 && selectionSim.graph[1].edges[0].weight==7);
    QApplication::sendEvent(&selection,&backspace);assert(selectionSim.graph.size()==2 && selectionSim.graph[0].edges.empty() && selectionSim.graph[1].edges.empty());
    // Segment context menus also use transformed coordinates after zoom.
    selectionSim.setGraphType(DijkstraSimulator::GraphType::Directed);selectionSim.setEdge(0,1,8,true);
    selection.zoomBy(1.2);app.processEvents();
    popup(&selection,QPoint(196,250),[&](QMenu *menu){auto *action=menu->findChild<QAction*>("toggleEdgeDirection");assert(action);action->trigger();});
    assert(selectionSim.graphType==DijkstraSimulator::GraphType::Mixed && !selectionSim.graph[0].edges[0].directed);
    selection.resetView();app.processEvents();
    // Pan and zoom then click the first vertex at its transformed screen location.
    sendMouse(QEvent::MouseButtonPress,{400,250},Qt::MiddleButton,Qt::MiddleButton);
    sendMouse(QEvent::MouseMove,{430,270},Qt::NoButton,Qt::MiddleButton);
    sendMouse(QEvent::MouseButtonRelease,{430,270},Qt::MiddleButton,Qt::NoButton);
    selection.zoomBy(1.2);app.processEvents();
    sendMouse(QEvent::MouseButtonPress,{34,274},Qt::LeftButton,Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease,{34,274},Qt::LeftButton,Qt::NoButton);
    QApplication::sendEvent(&selection,&backspace);assert(selectionSim.graph.size()==1 && selectionSim.graph[0].edges.empty());
    selection.close();
    // Parallel edges retain independent identities, edits and shortest routes.
    for(bool directedEdge:{false,true}) {
        DijkstraSimulator multi;
        multi.addNode(100,250);multi.addNode(600,250);multi.targetNode=-1;
        assert(!multi.addEdge(0,0,1,directedEdge) && !multi.addEdge(-1,1,1,directedEdge));
        assert(!multi.addEdge(0,1,-1,directedEdge) && !multi.addEdge(0,1,10000,directedEdge));
        for(int weight:{9,2,7}) assert(multi.addEdge(0,1,weight,directedEdge));
        const int first=multi.graph[0].edges[0].id, best=multi.graph[0].edges[1].id;
        const int last=multi.graph[0].edges[2].id;
        assert(first!=best && first!=last && best!=last);
        assert(multi.graph[1].edges.size()==(directedEdge?0:3));
        multi.runToEnd();assert(multi.dist[1]==2 && multi.prevEdgeIds[1]==best);
        multi.stepBack();multi.step();assert(multi.prevEdgeIds[1]==best);
        assert(multi.editEdge(0,first,1,directedEdge));multi.runToEnd();
        assert(multi.dist[1]==1 && multi.prevEdgeIds[1]==first);
        assert(multi.removeEdgeById(first));multi.runToEnd();
        assert(multi.dist[1]==2 && multi.prevEdgeIds[1]==best);
        multi.startNode=1;multi.reset();multi.runToEnd();
        assert(multi.dist[0]==(directedEdge?std::numeric_limits<int>::max():2));
        if(!directedEdge) assert(multi.prevEdgeIds[0]==best);
    }
    // Removing one arrow preserves the opposite edge, including its own
    // weight, hit target, route identity and delete action.
    DijkstraSimulator parallelSim;
    parallelSim.addNode(100,250);parallelSim.addNode(600,250);
    parallelSim.graphType=DijkstraSimulator::GraphType::Mixed;
    parallelSim.setEdge(0,1,7,true);parallelSim.setEdge(1,0,11,true);
    const int undirectedId=parallelSim.graph[0].edges[0].id;
    const int reverseId=parallelSim.graph[1].edges[0].id;
    GraphWidget parallelView(&parallelSim);parallelView.resize(800,500);parallelView.show();app.processEvents();
    const auto canvasImage=parallelView.grab().toImage();
    for(const QPoint corner:{QPoint(0,0),QPoint(canvasImage.width()-1,0),QPoint(0,canvasImage.height()-1),QPoint(canvasImage.width()-1,canvasImage.height()-1)})
        assert(canvasImage.pixelColor(corner)==QColor("#f1f5f9"));
    popup(&parallelView,QPoint(400,262),[&](QMenu *menu){auto *action=menu->findChild<QAction*>("toggleEdgeDirection");assert(action);action->trigger();});
    assert(parallelSim.graph[0].edges.size()==1 && parallelSim.graph[1].edges.size()==2);
    assert(!parallelSim.graph[0].edges[0].directed && parallelSim.graph[0].edges[0].weight==7);
    assert(parallelSim.graph[1].edges[0].id==reverseId && parallelSim.graph[1].edges[0].directed && parallelSim.graph[1].edges[0].weight==11);
    QMouseEvent parallelEdit(QEvent::MouseButtonDblClick,QPointF(400,262),parallelView.mapToGlobal(QPoint(400,262)),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(&parallelView,&parallelEdit);app.processEvents();
    auto *parallelWeight=parallelView.findChild<QSpinBox*>("inlineEdgeWeight");assert(parallelWeight->isVisible() && parallelWeight->value()==7);
    parallelWeight->setValue(5);QApplication::sendEvent(parallelWeight,&weightConfirm);
    assert(parallelSim.graph[1].edges[0].weight==11 && parallelSim.graph[1].edges[1].weight==5);
    assert(parallelSim.editEdge(1,reverseId,2,true));
    parallelSim.startNode=1;parallelSim.reset();parallelSim.runToEnd();
    assert(parallelSim.dist[0]==2 && parallelSim.prevEdgeIds[0]==reverseId);
    parallelSim.stepBack();parallelSim.step();assert(parallelSim.prevEdgeIds[0]==reverseId);
    PathPreview parallelPreview(&parallelSim);parallelPreview.setVertex(0);parallelPreview.show();app.processEvents();
    parallelPreview.grab().save("checks/preview-parallel-route.png");parallelPreview.hide();
    // Both original edges can also independently become undirected.
    assert(parallelSim.editEdge(1,reverseId,2,false));
    assert(parallelSim.graph[0].edges.size()==2 && parallelSim.graph[1].edges.size()==2);
    parallelSim.reset();parallelSim.runToEnd();assert(parallelSim.dist[0]==2 && parallelSim.prevEdgeIds[0]==reverseId);
    app.processEvents();parallelView.grab().save("checks/preview-parallel-undirected.png");
    popup(&parallelView,QPoint(400,262),[&](QMenu *menu){auto *action=menu->findChild<QAction*>("deleteEdge");assert(action);action->trigger();});
    assert(parallelSim.graph[0].edges.size()==1 && parallelSim.graph[1].edges.size()==1);
    assert(parallelSim.graph[0].edges[0].id==reverseId && parallelSim.graph[0].edges[0].weight==2);
    assert(parallelSim.removeEdgeById(reverseId) && !parallelSim.removeEdgeById(undirectedId));
    assert(parallelSim.graph[0].edges.empty() && parallelSim.graph[1].edges.empty());parallelView.close();
    // Context actions apply to selected edges only, even with selected vertices
    // and an undirected clicked edge among directed selected siblings.
    DijkstraSimulator batchSim;
    batchSim.addNode(100,250);batchSim.addNode(350,250);batchSim.addNode(600,250);
    batchSim.graphType=DijkstraSimulator::GraphType::Mixed;
    batchSim.setEdge(0,1,3,true);batchSim.setEdge(1,0,9,true);batchSim.setEdge(1,2,4,true);
    const int batchForward=batchSim.graph[0].edges[0].id,batchReverse=batchSim.graph[1].edges[0].id;
    const int batchOutside=batchSim.graph[1].edges[1].id;
    GraphWidget batchView(&batchSim);batchView.resize(800,500);batchView.show();app.processEvents();
    auto batchMouse=[&](QEvent::Type type,QPoint point,Qt::MouseButton button,Qt::MouseButtons buttons,Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
        QMouseEvent event(type,QPointF(point),batchView.mapToGlobal(point),button,buttons,modifiers);
        QApplication::sendEvent(&batchView,&event);app.processEvents();
    };
    popup(&batchView,QPoint(233,262),[&](QMenu *menu){menu->findChild<QAction*>("toggleEdgeDirection")->trigger();});
    batchMouse(QEvent::MouseButtonPress,{180,210},Qt::LeftButton,Qt::LeftButton);
    batchMouse(QEvent::MouseMove,{280,290},Qt::NoButton,Qt::LeftButton);
    batchMouse(QEvent::MouseButtonRelease,{280,290},Qt::LeftButton,Qt::NoButton);
    batchMouse(QEvent::MouseButtonPress,{65,250},Qt::LeftButton,Qt::LeftButton,Qt::ControlModifier);
    batchMouse(QEvent::MouseButtonRelease,{65,250},Qt::LeftButton,Qt::NoButton,Qt::ControlModifier);
    popup(&batchView,QPoint(233,262),[&](QMenu *menu){
        auto *action=menu->findChild<QAction*>("toggleEdgeDirection");assert(action && action->text()=="Xóa hướng");action->trigger();
    });
    for(const auto &node:batchSim.graph) for(const auto &edge:node.edges) {
        if(edge.id==batchOutside) assert(edge.directed && edge.weight==4);
        else assert(!edge.directed && edge.weight==(edge.id==batchForward?3:9));
    }
    batchSim.setGraphType(DijkstraSimulator::GraphType::Directed);batchView.update();app.processEvents();
    popup(&batchView,QPoint(233,262),[&](QMenu *menu){menu->findChild<QAction*>("toggleEdgeDirection")->trigger();});
    for(const auto &node:batchSim.graph) for(const auto &edge:node.edges)
        assert(edge.directed==(edge.id==batchOutside));
    // Right-clicking outside the selection acts only on that edge.
    popup(&batchView,QPoint(568,250),[&](QMenu *menu){menu->findChild<QAction*>("toggleEdgeDirection")->trigger();});
    assert(batchSim.graph[0].edges.size()==2 && batchSim.graph[2].edges.size()==1);
    popup(&batchView,QPoint(233,262),[&](QMenu *menu){menu->findChild<QAction*>("deleteEdge")->trigger();});
    assert(batchSim.graph.size()==3 && batchSim.graph[0].edges.empty());
    assert(batchSim.graph[1].edges.size()==1 && batchSim.graph[1].edges[0].id==batchOutside);
    assert(batchSim.graph[2].edges.size()==1 && batchSim.graph[2].edges[0].id==batchOutside);
    assert(!batchSim.removeEdgeById(batchForward) && !batchSim.removeEdgeById(batchReverse));
    QApplication::sendEvent(&batchView,&backspace);assert(batchSim.graph.size()==2);batchView.close();
    MainWindow window; window.show(); app.processEvents();
    // Three or more parallel edges curve into their nodes; labels and line
    // menus must resolve the actual curve rather than its straight chord.
    for(bool directedCurve:{false,true}) {
        DijkstraSimulator curves;
        curves.addNode(100,250);curves.addNode(600,250);
        for(int weight:{3,5,7,9}) curves.addEdge(0,1,weight,directedCurve);
        const int topId=curves.graph[0].edges[0].id;
        GraphWidget curveView(&curves);curveView.resize(800,500);curveView.show();app.processEvents();
        curveView.grab().save(directedCurve?"checks/curves-directed.png":"checks/curves-undirected.png");
        QMouseEvent editCurve(QEvent::MouseButtonDblClick,QPointF(400,289),curveView.mapToGlobal(QPoint(400,289)),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&curveView,&editCurve);app.processEvents();
        auto *curveWeight=curveView.findChild<QSpinBox*>("inlineEdgeWeight");
        assert(curveWeight->isVisible() && curveWeight->value()==3);
        curveWeight->setValue(4);QApplication::sendEvent(curveWeight,&weightConfirm);app.processEvents();
        assert(curves.graph[0].edges[0].weight==4 && curves.graph[0].edges[1].weight==5);
        popup(&curveView,QPoint(244,280),[&](QMenu *menu){
            auto *action=menu->findChild<QAction*>("deleteEdge");assert(action);action->trigger();
        });
        assert(curves.graph[0].edges.size()==3 && !curves.removeEdgeById(topId));
        app.processEvents();curveView.grab().save(directedCurve?"checks/curves-three-directed.png":"checks/curves-three-undirected.png");
        // A marquee around a curved segment excludes the straight chord.
        auto curveMouse=[&](QEvent::Type type,QPoint point,Qt::MouseButton button,Qt::MouseButtons buttons) {
            QMouseEvent event(type,QPointF(point),curveView.mapToGlobal(point),button,buttons,Qt::NoModifier);
            QApplication::sendEvent(&curveView,&event);app.processEvents();
        };
        const int bottomId=curves.graph[0].edges[0].id;
        curveMouse(QEvent::MouseButtonPress,{230,266},Qt::LeftButton,Qt::LeftButton);
        curveMouse(QEvent::MouseMove,{260,274},Qt::NoButton,Qt::LeftButton);
        curveMouse(QEvent::MouseButtonRelease,{260,274},Qt::LeftButton,Qt::NoButton);
        QApplication::sendEvent(&curveView,&backspace);app.processEvents();
        assert(curves.graph.size()==2 && curves.graph[0].edges.size()==2);
        assert(!curves.removeEdgeById(bottomId));
        curveView.close();
    }
    assert(window.width() <= window.screen()->availableGeometry().width());
    assert(window.height() <= window.screen()->availableGeometry().height());
    auto *windowViewport=window.findChild<QScrollArea*>("windowViewport");assert(windowViewport);
    window.resize(640,480);app.processEvents();
    assert(window.size()==QSize(640,480));
    assert(windowViewport->horizontalScrollBar()->maximum()>0);
    assert(windowViewport->verticalScrollBar()->maximum()>0);
    windowViewport->verticalScrollBar()->setValue(windowViewport->verticalScrollBar()->maximum());
    app.processEvents();window.grab().save("checks/small-screen-controls.png");
    windowViewport->verticalScrollBar()->setValue(0);
    window.resize(1280,820);app.processEvents();
    auto *tabs=window.findChild<QTabWidget*>("detailTabs");
    auto *graph=window.findChild<GraphWidget*>();
    auto *code=window.findChild<QTextEdit*>();
    QPushButton *next=nullptr,*toggle=nullptr;
    for(auto *b:window.findChildren<QPushButton*>()) {
        if(b->text().startsWith(QString::fromUtf8("Bước tiếp"))) next=b;
        if(b->isCheckable()) toggle=b;
    }
    assert(tabs && graph && code && next && toggle && tabs->currentIndex()==0);
    auto *targets=window.findChild<QComboBox*>("targetSelector");auto *sources=window.findChild<QComboBox*>("sourceSelector");
    assert(targets && sources && targets->itemText(0)=="Mọi đỉnh");
    targets->setCurrentIndex(0);app.processEvents();assert(targets->currentData().toInt()==-1);
    targets->setCurrentIndex(6);app.processEvents();
    window.findChild<QPushButton*>("zoomIn")->click();assert(graph->zoomFactor()>1);
    auto *zoomButton=window.findChild<QPushButton*>("resetZoom");assert(zoomButton->text()=="120%");
    window.findChild<QPushButton*>("resetZoom")->click();assert(graph->zoomFactor()==1);
    assert(zoomButton->text()=="100%");
    QWheelEvent zoomWheel(QPointF(100,100),graph->mapToGlobal(QPoint(100,100)),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    QApplication::sendEvent(graph,&zoomWheel);assert(zoomButton->text()=="115%");
    graph->zoomBy(100);assert(zoomButton->text()=="400%");
    graph->zoomBy(0.001);assert(zoomButton->text()=="20%");
    graph->resetLayout();assert(zoomButton->text()=="100%");app.processEvents();
    QDir().mkpath("checks");
    auto *templates=window.findChild<QComboBox*>("graphTemplates");
    assert(templates->count() == 6 && templates->itemText(5) == "Bảng trắng");
    assert(templates->itemText(1)=="Tuyến tính" && templates->itemText(2)=="Cạnh song song" && templates->itemText(4)=="Đồ thị lưới");
    auto *graphType=window.findChild<QComboBox*>("edgeKindSelector");assert(graphType && graphType->count()==3);
    graphType->setCurrentIndex(1);app.processEvents();assert(templates->currentIndex()==-1);
    window.grab().save("checks/preview-graph-type-directed.png");
    graphType->setCurrentIndex(2);app.processEvents();assert(graphType->currentIndex()==2);
    graphType->setCurrentIndex(0);app.processEvents();assert(graphType->currentIndex()==0);
    templates->setCurrentIndex(5); app.processEvents();
    assert(window.findChild<QTableWidget*>()->rowCount() == 0);
    window.grab().save("checks/preview-blank-template.png");
    templates->setCurrentIndex(0); app.processEvents();
    for(const QSize size : {QSize(1280,820),QSize(980,720)}) {
        window.resize(size); app.processEvents();
        for(int t=0;t<5;++t) {
            templates->setCurrentIndex(t); app.processEvents();
            tabs->setCurrentIndex(0); app.processEvents();
            const QRect before=graph->geometry();
            toggle->click(); app.processEvents();
            assert(tabs->currentIndex()==1 && toggle->isChecked());
            assert(graph->geometry()==before);
            assert(graph->height()>=260);
            auto *viewport=window.findChild<QScrollArea*>("graphViewport");
            auto *legend=window.findChild<QLabel*>("graphLegend");
            assert(viewport && legend);
            assert(viewport->mapToGlobal(QPoint(0,viewport->height())).y()<=legend->mapToGlobal(QPoint(0,0)).y());
            assert(code->horizontalScrollBar()->maximum()==0);
            window.grab().save(QString("checks/code-%1-%2.png").arg(size.width()).arg(t));
            tabs->setCurrentIndex(0); app.processEvents();
            assert(!toggle->isChecked() && graph->geometry()==before);
        }
    }
    window.resize(1280,820); templates->setCurrentIndex(0); toggle->click(); app.processEvents();
    for(int i=0;i<9;++i) next->click();
    assert(code->extraSelections().size()==1);
    assert(code->extraSelections()[0].cursor.blockNumber()==10);
    window.grab().save("checks/preview-code.png");
    window.resize(980,720); app.processEvents();
    window.grab().save("checks/preview-small.png");
    for(int i=0;i<100;++i) next->click();
    app.processEvents(); window.grab().save("checks/preview-finished.png");
    std::cout << "Code tabs preserve graph size on all templates; code highlight passed\n";
    auto *edit = window.findChild<QPushButton*>("editGraph");
    auto *random = window.findChild<QPushButton*>("randomGraph");
    assert(edit && random);
    assert(edit->text() == "Insert");
    QFile txt("checks/import.txt"); assert(txt.open(QIODevice::WriteOnly));
    txt.write("a b 10\nc d 20\na c 10\n"); txt.close();
    QTimer::singleShot(0, [&] {
        auto *dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget()); assert(dialog);
        dialog->selectFile(QDir::current().absoluteFilePath("checks/import.txt"));
        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    edit->click(); app.processEvents();
    assert(templates->currentIndex() == -1 && window.findChild<QTableWidget*>()->rowCount() == 4);
    auto *speed = window.findChild<QSlider*>("speedSlider"); assert(speed);
    speed->setValue(speed->maximum()); app.processEvents();
    auto *table = window.findChild<QTableWidget*>();
    assert(table->item(0,1)->text() == "0" && table->item(1,1)->text() == "10" && table->item(2,1)->text() == "10" && table->item(3,1)->text() == "30");
    assert(!next->isEnabled());
    window.grab().save("checks/preview-import-instant.png");
    auto hoverRow=[&](int row) {
        const QPoint point=table->visualItemRect(table->item(row,0)).center();
        QMouseEvent move(QEvent::MouseMove,QPointF(point),table->viewport()->mapToGlobal(point),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(table->viewport(),&move);app.processEvents();
    };
    auto *hoverPreview=static_cast<PathPreview*>(window.findChild<QWidget*>("pathPreview"));assert(hoverPreview);
    hoverRow(3);assert(hoverPreview->isVisible() && hoverPreview->vertex()==3 && targets->currentData().toInt()==-1);
    hoverPreview->grab().save("checks/preview-hover-route.png");
    hoverRow(0);assert(hoverPreview->vertex()==0);
    QEvent leaveTable(QEvent::Leave);QApplication::sendEvent(table->viewport(),&leaveTable);assert(!hoverPreview->isVisible());
    targets->setCurrentIndex(1);app.processEvents();hoverRow(0);assert(!hoverPreview->isVisible());
    QFile arrowFile("checks/import-arrow.txt");assert(arrowFile.open(QIODevice::WriteOnly));
    arrowFile.write("a -> b 10\nb -> a 20\nb -- c 0\nx -> y 7\n");arrowFile.close();
    QTimer::singleShot(0,[&]{
        auto *dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());assert(dialog);
        dialog->selectFile(QDir::current().absoluteFilePath("checks/import-arrow.txt"));QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
    });
    edit->click();app.processEvents();next->click();app.processEvents();
    assert(targets->currentData().toInt()==-1 && table->rowCount()==5);
    assert(table->item(1,1)->text()=="10" && table->item(2,1)->text()=="10" && table->item(3,1)->text()==QString::fromUtf8("∞"));
    hoverRow(2);assert(hoverPreview->isVisible() && hoverPreview->vertex()==2);hoverPreview->grab().save("checks/preview-hover-directed-route.png");
    hoverRow(3);assert(hoverPreview->isVisible() && hoverPreview->vertex()==3);hoverPreview->grab().save("checks/preview-hover-unreachable.png");
    QApplication::sendEvent(table->viewport(),&leaveTable);assert(!hoverPreview->isVisible());
    speed->setValue(1100);QMetaObject::invokeMethod(&window,"onReset",Qt::DirectConnection);
    for(int i=0;i<9;++i) next->click();app.processEvents();
    assert(table->item(1,1)->text()=="10" && table->item(1,0)->foreground().color()!=QColor("#0f766e"));
    hoverRow(1);assert(hoverPreview->isVisible());hoverPreview->grab().save("checks/preview-hover-pending-route.png");
    for(int i=0;i<6;++i) next->click();app.processEvents();
    assert(hoverPreview->isVisible() && hoverPreview->vertex()==1 && targets->currentData().toInt()==-1);
    assert(table->item(1,0)->foreground().color()==QColor("#0f766e"));
    hoverPreview->grab().save("checks/preview-hover-settled-route.png");
    QApplication::sendEvent(table->viewport(),&leaveTable);
    speed->setValue(1100);
    graphType->setCurrentIndex(0);
    templates->setCurrentIndex(0); app.processEvents();
    speed->setValue(speed->maximum()); app.processEvents();
    assert(table->item(5,1)->text() == "13" && !next->isEnabled());
    window.grab().save("checks/preview-target-instant.png");
    speed->setValue(1100);
    for(int style=0; style<4; ++style) {
        QTimer::singleShot(0, [&, style] {
            auto *dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget()); assert(dialog);
            dialog->findChild<QSpinBox*>("randomNodeCount")->setValue(12);
            dialog->findChild<QComboBox*>("randomLayout")->setCurrentIndex(style);
            dialog->grab().save("checks/preview-generator.png");
            if(style == 0) {
                auto *combo=dialog->findChild<QComboBox*>("randomLayout");
                combo->showPopup(); app.processEvents(); combo->view()->grab().save("checks/preview-layout-options.png"); combo->hidePopup();
            }
            dialog->accept();
        });
        random->click(); app.processEvents();
        assert(window.findChild<QTableWidget*>()->rowCount() == 12);
        assert(templates->currentIndex() == -1);
        window.grab().save(QString("checks/preview-random-%1.png").arg(style));
    }

    QTimer::singleShot(0,[&]{
        auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        dialog->findChild<QSpinBox*>("randomNodeCount")->setValue(12);
        auto *type=dialog->findChild<QComboBox*>("randomGraphType");assert(type && type->count()==2);type->setCurrentIndex(1);
        dialog->grab().save("checks/preview-generator-directed.png");dialog->accept();
    });
    random->click();app.processEvents();assert(graphType->currentIndex()==1);
    QTimer::singleShot(0,[&]{
        auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        dialog->findChild<QSpinBox*>("randomNodeCount")->setValue(12);dialog->findChild<QComboBox*>("randomGraphType")->setCurrentIndex(0);dialog->accept();
    });
    random->click();app.processEvents();assert(graphType->currentIndex()==0);
    assert(!window.findChild<QPushButton*>("deleteVertex"));
    for(int remaining=12;remaining>0;--remaining) {
        assert(graph->removeVertex(0)); app.processEvents();
        assert(window.findChild<QTableWidget*>()->rowCount()==remaining-1);
    }
    window.grab().save("checks/preview-empty.png");
    QTimer::singleShot(0, [&] {
        auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        auto *style=dialog->findChild<QComboBox*>("randomLayout");assert(style && style->count()==4);
        for(int i=0;i<style->count();++i) assert(style->itemText(i)!="Bảng trắng" && style->itemData(i).toInt()>=0);
        assert(dialog->findChild<QSpinBox*>("randomNodeCount")->isEnabled());
        dialog->reject();
    });
    random->click();app.processEvents();assert(window.findChild<QTableWidget*>()->rowCount()==0);
    templates->setCurrentIndex(5);app.processEvents();
    assert(templates->currentIndex()==5 && window.findChild<QTableWidget*>()->rowCount()==0);
    popup(graph,QPoint(150,150),[&](QMenu *menu) { menu->actions()[0]->trigger(); });
    assert(window.findChild<QTableWidget*>()->rowCount() == 1);
    next->click(); app.processEvents();
    const QImage beforeCancel = graph->grab().toImage();
    QTimer::singleShot(0, [&] { qobject_cast<QDialog*>(QApplication::activeModalWidget())->reject(); });
    random->click(); app.processEvents(); assert(graph->grab().toImage() == beforeCancel);
    auto runTimed=[&](int speedValue) {
        auto *play=window.findChild<QPushButton*>("playBtn");speed->setValue(speedValue);
        QElapsedTimer clock;clock.start();play->click();
        QEventLoop loop;QTimer timeout;timeout.setSingleShot(true);
        QTimer poll;poll.setInterval(5);
        QObject::connect(&timeout,&QTimer::timeout,&loop,&QEventLoop::quit);
        QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(!next->isEnabled()) loop.quit();});
        timeout.start(5000);poll.start();loop.exec();
        assert(!next->isEnabled());return clock.elapsed();
    };
    templates->setCurrentIndex(0);app.processEvents();
    const auto sixMs=runTimed(speed->maximum()-1);
    QTimer::singleShot(0,[&]{
        auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        dialog->findChild<QSpinBox*>("randomNodeCount")->setValue(30);dialog->accept();
    });
    random->click();app.processEvents();
    const auto thirtyMs=runTimed(speed->maximum()-1);
    assert(sixMs>600 && sixMs<3000 && thirtyMs>600 && thirtyMs<3000);
    assert(std::abs(sixMs-thirtyMs)<500);
    std::cout << "Normalized playback: 6 nodes " << sixMs << " ms; 30 nodes " << thirtyMs << " ms\n";
    templates->setCurrentIndex(1);app.processEvents();
    const auto slowerMs=runTimed(1800);assert(slowerMs>sixMs*1.25);
    std::cout << "Exponential speed at 1800: " << slowerMs << " ms\n";
    std::cout << "Vertex insertion/deletion, dragging, context menus, keyboard weights, empty graphs, random layouts and dialogs passed\n";
}


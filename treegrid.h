#ifndef _treegrid_h_
#define _treegrid_h_

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

#include <vector>
#include <map>
#include <functional>
#include <utilfuncs/utilfuncs.h>
#include <exception>

//-----------------------------------------------------------------------------
//todo: make these variables (if needed (customizing))...
///todo: G_ROW_CY - should be tossed & the individual cy of rows calculted from max cell-heights (+ 1 or 2 pixel spacing between rows)
#define G_ROW_CY (int)24
#define G_SPACE (int)16
#define G_GAP (int)8
///todo: G_PIC_WIDTH...larger pics? ... height?
#define G_PIC_WIDTH (int)16
///todo: G_LINE_HEIGHT should depend on font-size...which will effect cell-height(=>G_ROW_CY)...
#define G_LINE_HEIGHT (int)16


//using different (light/faint) pastels for contained expanded nodes ...
#define G_PASTEL_ONE Color(255,255,235)
#define G_PASTEL_ONE_ALT Color(255,255,210)
#define G_PASTEL_TWO White()
#define G_PASTEL_TWO_ALT Color(235,235,235)
#define G_PASTEL_THREE Color(235,255,255)
#define G_PASTEL_THREE_ALT Color(210,255,255)

#define G_COL_FG_DEFAULT Black()
#define G_COL_BG_DEFAULT White()

#define G_COL_FG_FOCUS White()
#define G_COL_BG_FOCUS Color(75, 105, 131)

#define G_COL_FG_SELECTED Black()
#define G_COL_BG_SELECTED Color(255, 120, 100)


//-------------------------------------------------------------------------------
class TGTree;
class TreeGrid;
//--------------------------------------
class Cell
{
	std::string val;
	const Display *pDsp;
	
	Event<> when_resync;
	
public:
	void clear() { val.clear(); pDsp=nullptr; }
	
	virtual ~Cell();
	Cell();
	Cell(const std::string &v);
	Cell(const Cell &C);

	void operator=(const Cell &C);
	void SetDisplay(const Display &d);
	const Display&	GetDisplay() const;
	void SetData(std::string v);
	const std::string GetData() const;
	
	Event<> WhenCellChanged; //val-changed
	
	friend class TGTree;
	friend class TreeGrid;
};

typedef std::vector<Cell> Cells;


//---------------------------------------
struct node_pic_list : std::map<int, std::pair<Image, int> > //[id]=[image, usecount]
{
	~node_pic_list() { clear(); }
	node_pic_list() { clear(); }
	int Add(Image img)
	{
		if (img.IsNullInstance()) return 0;
		int i=1;
		for (auto& p:(*this)) { if (p.second.first==img) { p.second.second++; return p.first; } else if (i==p.first) i++; }
		(*this)[i]=std::pair<Image, int>(img, 1);
		return i;
	}
	void Drop(int pid=0)
	{
		if (!pid) return;
		auto it=find(pid);
		if (it!=end()) { if ((*it).second.second<=1) erase(it); else (*it).second.second--; }
	}
	bool Get(int pid, Image &img) { if (pid&&(find(pid)!=end())) { img=(*this)[pid].first; return true; } return false; }
	bool IncPic(int pid) { if (pid&&(find(pid)!=end())) { (*this)[pid].second++; return true; } return false; } //copied
};

extern node_pic_list node_pics; //?should be in Nodes? each? or in TGTree?-since it is used/displayed in the tree..


//---------------------------------------
struct Node;
typedef Node* PNode;

struct Nodes
{
	typedef std::vector<PNode> VP; VP vp{}; //sortable list
	using iterator=VP::iterator;
	using const_iterator=VP::const_iterator;
	using reverse_iterator=VP::reverse_iterator;
	using const_reverse_iterator=VP::const_reverse_iterator;

	iterator				begin()			{ return vp.begin(); }
	iterator				end()			{ return vp.end(); }
	const_iterator			begin() const	{ return vp.begin(); }
	const_iterator			end() const		{ return vp.end(); }
	reverse_iterator		rbegin()		{ return vp.rbegin(); }
	reverse_iterator		rend()			{ return vp.rend(); }
	const_reverse_iterator	rbegin() const	{ return vp.rbegin(); }
	const_reverse_iterator	rend() const	{ return vp.rend(); }
	
	void clear();
	bool empty() const;
	
	~Nodes();
	Nodes();
	Nodes(const Nodes &R);
	Nodes& operator=(const Nodes &R);

	size_t size() const;
	iterator find(const std::string &key);
	const_iterator find(const std::string &key) const;
	void erase(const std::string &k);

	PNode operator[](const std::string &key);
	const PNode operator[](const std::string &key) const;
	PNode from_index(size_t i);
	const PNode from_index(size_t i) const;
	PNode operator[](size_t i);
	const PNode operator[](size_t i) const;

	bool has_key(const std::string &k);
	PNode Add(PNode parent, const std::string &name, const std::string &key, Image pic=Image());
	void Remove(PNode pN);
	void Remove(const std::string &k);

};

struct Node
{
	
	////make cells fixed-size - same number as columns
	
	Cells cells{}; //cells[0]==leaf-name
	std::string skey;
	std::string sinfo;
	int y_pos, e_x_pos, p_x_pos, l_x_pos;
	int picid;
	bool b_selected;
	Nodes nodes;
	PNode parent;
	bool b_expandable;
	bool b_expanded;
	bool b_locked;

	void clear();

	~Node();
	Node();
	Node(PNode pParent, const std::string &name, const std::string &key, Image pic=Image());
	Node(const Node &N);
	Node& operator=(const Node &N);

	void select(bool b=true);

	Cell& CellAt(size_t idx);
	std::string GetCellData(size_t idx) const { if (idx<cells.size()) return cells[idx].GetData(); return ""; }
	void SetCellData(size_t idx, std::string s);
	void add_cell(){}
	template<typename...T> void add_cell0(size_t m, size_t &i) {}
	template<typename...T, typename H> void add_cell0(size_t m, size_t &i, H h, T...t)
		{
			i++;
			if (i<m) cells[i]=Cell(h);
			add_cell0(m, i, t...);
		}
	template<typename...T> void add_cell(T...t)
		{
			size_t i=0;
			add_cell0(cells.size(), i, t...);
		}
	void ClearCells();
	
	PNode AddNode(const std::string &name, const std::string &key="", Image pic=Image());
	void DropNode(PNode pN);
	void DropNode(const std::string &key);
	PNode GetNode(const std::string &key);
	std::string GetNameKey(const std::string &info);
	std::string GetInfoKey(const std::string &info);
	PNode GetNamedNode(const std::string &info, bool bDeep=true);
	PNode GetInfoNode(const std::string &info, bool bDeep=true);
	void ClearNodes();
	PNode Parent() const;
	size_t NodeCount() const;
	bool HasNodes() const;
	
	PNode SetInfo(const std::string &s);
	const std::string GetInfo() const;
	const std::string GetKey() const;
	bool IsSelected() const;

	PNode SetLocked(bool b);
	bool IsLocked() const;
	void Lock();
	void Unlock();

	bool IsExpandable() const;
	PNode Expandable(bool b=true);
	bool IsExpanded() const;
	bool CanExpand() const;
	void Expand();
	void Contract();
	
	bool GetPic(Image &img) const;
	void SetPic(const Image &img);

	int get_indent() const;
	
};

//-------------------------------------------------------------------------------
typedef std::function<bool(const PNode&, const PNode&)> CompareNodes;

//-------------------------------------------------------------------------------
class Column //: Moveable<Column>
{
	String title;
	int w, align;
	bool b_asc, b_sort;
	const Display *dsp;

public:
	CompareNodes Sorter;

	virtual ~Column();
	Column();
	Column(const Column &C);
	Column(const String &title, int width);
	
	Column operator=(const Column &C) { title=C.title; w=C.w; align=C.align; b_asc=C.b_asc; b_sort=C.b_sort; dsp=C.dsp; Sorter=C.Sorter; return *this; }

	Column& Sorting(CompareNodes sortfunc=0);//nullptr);
	bool IsSorting() const;
	bool IsASC() const;
	void ToggleSort();
	Column&	Align(int align);
	int		Align() const;
	Column&			SetDisplay(const Display &d);
	const Display&	GetDisplay() const;
	const String&	Title() const;
	void			Title(const String &title);
	int		Width() const;
	void	Width(int width);
};

typedef std::vector<Column> Columns;

//-------------------------------------------------------------------------------
class TGHeader : public Ctrl
{
	TreeGrid *owner;
	Columns cols;
	size_t CurCol, size_col;
	int left_down;
	bool b_is_sizing, b_show;
	void scroll_x(int xa); //xa == -horz-S/Bar-pos
	bool is_on_sizer(int p) const;
	int x_pos(size_t idx);
	bool col_at_x(int p, size_t &idx);
	bool set_cur_col(int p, size_t &idx); //p==x-coordinate of mouse-CLICK(&not IsSizer), idx set to col; True: curcol set
	int sort_pic_width(); //used for column-display adjustment in tree::paint()

	Event<size_t> WhenSorting;
	Event<> WhenColumnAdded;

public:
	typedef TGHeader CLASSNAME;
	virtual ~TGHeader();
	TGHeader();
	TGHeader(const TGHeader &T) { *this=T; }
	TGHeader operator=(const TGHeader &T) { cols=T.cols;  return *this; }

	void Clear();
	virtual void Paint(Draw &drw);
	void show(bool b);
	bool show();
	Image CursorImage(Point p, dword keyflags);
	virtual void LeftDown(Point p, dword keyflags);
	virtual void MouseMove(Point p, dword keyflags);
	virtual void LeftUp(Point p, dword keyflags);
	Column& Add(const String &title, int width);
	size_t ColumnCount() { return cols.size(); }
	Column* ColumnAt(size_t idx);
	int Width();
	bool IsSizer(int p, size_t &idx) const; //p==x-coordinate of mouse-pointer, idx set to col-on-left
	friend class TGTree;
	friend class TreeGrid;
};

//-------------------------------------------------------------------------------
enum EXDisp //EXpandDisposition
{
	EXD_NONE=0, //node-carrying nodes (node::nodes not empty) - no special treatment
	EXD_FIRST, //appear first in tree
	EXD_LAST, //last in tree
};

//-------------------------------------------------------------------------------
struct TGTree : public Ctrl
{
	TreeGrid *owner;
	Nodes roots;
	PNode pCurNode;
	PNode pSelPivot;
	int lines_total_height;
	bool bShowTreeLines, b_has_selected;
	EXDisp exd;
	VScrollBar SBV;
	HScrollBar SBH;
	size_t cur_column;
	
	int show_nodes(Draw &drw, Nodes &nodes, Size szV, int Y=0);
	void show_tree_lines(Draw &drw, PNode N, Size szV);
	void show_vert_grid(Draw &drw, Size szV);
	int set_tree_positions(Nodes &nodes, int Y=0);
	PNode get_node(PNode pN, const String &key);
	PNode get_last_node();
	bool check_node_key(String &key);
	PNode get_y_node(int Y, PNode N);
	PNode get_node_at_y(int Y, Nodes &nodes);
	bool toggle_expand(PNode N, Point pt);
	void check_cur_node_contract(PNode N);
	void sort_nodes(TGHeader *pH, Nodes &nodes, size_t c, bool b);
	void unselect_nodes(Nodes &nodes);
	size_t get_selected_keys(Nodes &nodes, std::vector<String> &V); //recursive, V must be cleared before first call
	size_t get_selected_nodes(Nodes &nodes, std::vector<PNode> &V); //recursive, V must be cleared before first call
	bool contains_locked_nodes(PNode N);
	Rect get_abs_tree_rect();
	size_t set_click_column(int x);
	bool point_in_tree(Point p);
	PNode prev_node(PNode N);
	PNode next_node(PNode N);
	void sync_nodes();

	PNode AddNode(PNode pParent, const String &treetext, const String &key);
	PNode AddNode(PNode pParent, Image pic, const String &treetext, const String &key);

	PNode get_node(Nodes &nodes, const std::string &k); //helper for GetNode()
	PNode GetNode(const String &key);
	PNode get_named_node(Nodes &nodes, const std::string &sname);
	PNode get_info_node(Nodes &nodes, const std::string &info); //helper for GetNode()
	PNode GetNamedNode(const String &sname);
	PNode GetInfoNode(const String &info);
	void SelectNode(PNode N, bool bSelect);
	void SelectNode(const String &key, bool bSelect);
	void SelectRange(PNode N1, PNode N2);
	void Lock(PNode N, bool b);
	void DropNode(PNode N);
	void DropNodes(PNode N); //clears nodes
	void Expand(PNode N, bool b=true);
	void FocusNode(PNode N, bool bView=true);
	void BringToView(PNode N);
	void SortNodes(TGHeader *pH, size_t colidx, bool bASC);

	Event<PNode, dword> WhenLeftDown;
	Event<PNode, size_t> WhenLeftDouble;
	Event<PNode> WhenRightDown;
	Event<PNode> WhenFocus;
	Event<PNode> WhenBeforeNodeExpand; //before expanding
	Event<PNode> WhenAfterNodeContract; //after contracting

	void Clear();
	virtual void Layout();
	virtual void Refresh();
	void Paint(Draw &drw);
	void DoVScroll();
	void DoHScroll();
	virtual bool Key(dword key, int);
	virtual void MouseWheel(Point p, int zdelta, dword keyflags);
	virtual void LeftDown(Point p, dword flags);
	virtual void LeftUp(Point p, dword flags);
	virtual void RightDown(Point p, dword flags);
	virtual void LeftDouble(Point p, dword flags);
	virtual void MouseMove(Point p, dword flags);
	
	Image CursorImage(Point p, dword keyflags);

//public:
	typedef TGTree CLASSNAME;

	TGTree();
	virtual ~TGTree();
	TGTree(const TGTree &T) { *this=T; }

	TGTree operator=(const TGTree &T) { owner=T.owner; roots=T.roots; bShowTreeLines=T.bShowTreeLines; exd=T.exd; return *this; }

	friend class TGHeader;
	friend class TreeGrid;
};


//-------------------------------------------------------------------------------
class TreeGrid : public Ctrl
{
	typedef TreeGrid CLASSNAME;

	TGHeader header;
	TGTree tree;
	size_t sort_col;
	bool b_vert_grid, b_internal; //b_internal: pad cells to column::count - see AddNode() and <>AddNode()
	bool b_add_tg_help{true};
	
	void check_cells(PNode N);
	void sync_all_node_cells(Nodes &nodes);
	void FillMenuBar(MenuBar &menubar);
//	void OnTGHelp();
	void ShowPopMenu(Point p);
	void OnBNE(PNode N);
	void OnANC(PNode N);
	void OnLeftDown(PNode N, dword flags);
	void OnLeftDouble(PNode N, size_t cellidx);
	void OnRightDown(PNode);// N);

public:
	virtual ~TreeGrid();
	TreeGrid();
	TreeGrid(const TreeGrid &T) { *this=T; }

	TreeGrid operator=(const TreeGrid &T) { header=T.header; tree=T.tree; sort_col=T.sort_col; b_vert_grid=T.b_vert_grid; b_internal=T.b_internal; b_add_tg_help=T.b_add_tg_help; return *this; }

	virtual void Layout();
	virtual void GotFocus();
	
	void Clear();
	void ClearTree();

	Column& AddColumn(const String title, int width);
	void SetColumnWidth(int c, int w) { Column* pc=header.ColumnAt(c); if (pc) pc->Width(w); }

	PNode AddNode(PNode Parent, const String &name, const String &key="");
	template<typename...CellData> PNode AddNode(PNode Parent, const String &name, const String &key, CellData...celldata)
	{
		PNode N=tree.AddNode(Parent, name, key);
		if (N)
		{
			check_cells(N);
			N->add_cell(celldata...);
			SyncNodes();
		}
		return N;
	}
	template<typename...CellData> PNode AddNode(PNode Parent, Image pic, const String &name, const String &key, CellData...celldata)
	{
		PNode N=tree.AddNode(Parent, pic, name, key);
		if (N)
		{
			check_cells(N);
			N->add_cell(celldata...);
			SyncNodes();
		}
		return N;
	}

	void RefreshTreeGrid();
	void DeleteNode(PNode N, bool bfocus=true);
	PNode GetNode(const String &key);
	PNode GetNodeNamed(const String &S);
	PNode GetInfoNode(const String &info);
	size_t RootCount();
	PNode RootAt(size_t idx);
	void LockNode(PNode N, bool bExpand=true);
	void UnlockNode(PNode N);
	void Expand(PNode N, bool b=true);
	void ClearNodes(PNode N);
	void SetFocusNode(PNode N, bool b=true);
	PNode GetFocusNode();
	size_t GetFocusCell();
	PNode GetPrevNode(PNode N);
	PNode GetNextNode(PNode N);
	void Select(PNode N, bool bSelect=true);
	void SelectRange(PNode N1, PNode N2);
	void ClearSelection();
	size_t GetSelectionKeys(std::vector<String> &vSel);
	size_t GetSelectionNodes(std::vector<PNode> &v);
	virtual bool Key(dword key, int r);
	void OnSorting(size_t idx);
	size_t GetSortColumnID();
	bool IsSortASC();
	void SyncNodes();
	bool NodeHasCell(PNode N, size_t cellidx);
	Cell& GetCell(PNode N, size_t cellidx);

	//----------------------------------customizing:
	void ListExpandingNodes(EXDisp exd); //EXD_FIRST, .._NONE, .._LAST
	EXDisp ListExpandingNodes();
	void ShowTreeLines(bool bShow);
	bool ShowTreeLines();
	void ShowHeader(bool bShow);
	bool ShowHeader();

	//----------------------------------
	Event<PNode> WhenFocus;
	Event<PNode, size_t> WhenNodeCellActivate;
	Event<MenuBar&> WhenMenuBar;
	Event<> WhenHelp;
	Event<PNode> WhenBeforeNodeExpand; //before expanding
	Event<PNode> WhenAfterNodeContract; //after contracting

	//----------------------------------
	void UseTGHelpMenu(bool b=true);
	
	//----------------------------------
	friend class TGHeader;
	friend class TGTree;
};



#endif //#ifndef _treegrid_h_


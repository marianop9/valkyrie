#include "Application.hpp"

wxIMPLEMENT_APP(MainApp);

bool MainApp::OnInit()
{
	wxWindowID mainFrameID = wxWindow::NewControlId();
	MainFrame* frame = new MainFrame(mainFrameID, "Valkyrie", wxDefaultPosition, wxSize(800, 600));
	frame->Show();
	return true;
}

MainFrame::MainFrame(wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(nullptr, id, title, pos, size)
{	
	// crea la lista y la conecta al evento de click en una columna
	this->mainListView = new MainList(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VRULES | wxLC_HRULES);
	// al seleccionar un item en la gui guarda el id del item y lo pasa al panel de los movimientos 
	Bind(wxEVT_LIST_ITEM_SELECTED, &MainFrame::setSelectedItem, this);
	
	
	mainListView->populateStock();
	// cargar items desde archivo .bin
	mainListView->populateList(getHead());

	wxPanel *buttonPanel = new wxPanel(this, wxID_ANY);

	wxButton *addButton = new wxButton(buttonPanel, wxID_ADD);
	wxButton *saveButton = new wxButton(buttonPanel, wxID_SAVE);
	wxButton *deleteButton = new wxButton(buttonPanel, wxID_DELETE);
	
	// sizer para los botones
	wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(deleteButton, 0, wxALL, 5);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(addButton, 0, wxALL, 5);
	buttonSizer->Add(saveButton, 0, wxALL, 5);

	buttonPanel->SetSizerAndFit(buttonSizer);

	// panel para registrar los movimientos
	stockMovementPanel = new StockMovementPanel(this);

	// sizer principal, agrega la lista y los botones
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(mainListView, 3, wxEXPAND);
	mainSizer->Add(stockMovementPanel, 0, wxEXPAND);
	mainSizer->Add(buttonPanel, 0, wxEXPAND);
	
	// se ajusta el sizer al panel principal

	this->SetSizerAndFit(mainSizer);
}



void MainFrame::setSelectedItem(wxListEvent &evt)
{
	selectedItemID = evt.GetText();
	selectedItemIndex = evt.GetIndex();
	stockMovementPanel->appendID(selectedItemID);
}
const wxString &MainFrame::getSelectedItemID() const
{
	return selectedItemID;
}
long MainFrame::getSelectedItemIndex() const 
{
	return selectedItemIndex;
}
wxString MainFrame::getSelectedItemName() const
{
	return mainListView->GetItemText(selectedItemIndex, 1);
}

void MainFrame::onAddItemButton(wxCommandEvent &evt)
{
	addItemDialog = new AddItemDialog(this, wxID_ANY, wxDefaultPosition, wxSize(700, 300));
	addItemDialog->Bind(wxEVT_BUTTON, 
						[this](wxCommandEvent &evt) {
							// obtiene los datos ingresados en el dialogo
							ItemData newItemData = addItemDialog->onApplyButton();
							// agrega el item a la LL y a la listView
							this->mainListView->addNewItem(newItemData.id, newItemData.name, newItemData.stock, newItemData.price);
						},
						wxID_APPLY);
	// muestra el dialogo
	addItemDialog->ShowModal();
	// limpia la memoria asignada al dialogo
	addItemDialog->Destroy();
}
void MainFrame::onApplyMovementButton(wxCommandEvent &evt)
{
	// obtener los datos ingresados
	unsigned int id = stockMovementPanel->getID();
	int movement = stockMovementPanel->getMovement();
	// // encontrar el puntero del item asociado al ID ingresado.
	// // si no se encuentra termina
	Item* item = mainListView->findItem(id);

	if (item == NULL) {
		ErrorBox::callError(ERR_NOT_FOUND);
		printf("item not found\n"); 
		return;
	}
	// registra y checkea el movimiento
	if (registerMovement(item, movement) == 0) {
		ErrorBox::callError(ERR_INSUFFICIENT_STOCK);
		printf("error, insufficient stock\n"); 
		return;
	}

	// luego busca el item en la lista y lo borra
	// cada item en la GUI tiene metadata asociada a su puntero en la memoria del programa
	long itemIndex = mainListView->FindItem(-1, (wxUIntPtr) item);
	mainListView->DeleteItem(itemIndex);	
	// mostrar nuevamente el item modificado
	mainListView->addListViewItem(item);
	stockMovementPanel->clearMovementEntry();
}
void MainFrame::onSaveButton(wxCommandEvent &evt)
{
	wxDialog *saveDialog = new wxDialog(this, wxID_ANY, "Save...");

	wxTextCtrl *message = new wxTextCtrl(saveDialog, wxID_ANY, "Save changes?", wxDefaultPosition, wxDefaultSize, wxTE_CENTER | wxTE_READONLY);

	wxSizer *buttonSizer = saveDialog->CreateButtonSizer(wxCANCEL | wxYES);
	saveDialog->Bind(wxEVT_BUTTON, 
						[this, &saveDialog](wxCommandEvent &evt) {
							this->mainListView->saveList();
							saveDialog->Destroy();
						},
						wxID_YES);

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(message, 1, wxEXPAND | wxALL, 10);
	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
	saveDialog->SetSizerAndFit(mainSizer);

	saveDialog->ShowModal();

	saveDialog->Destroy();
}
void MainFrame::onDeleteButton(wxCommandEvent &evt)
{
	// checkear que el usuario haya selecionado un item
	if(mainListView->GetSelectedItemCount() < 1)
	{								
		ErrorBox::callError(ERR_NOT_SELECTED);
		printf("no item selected\n");
		return;
	}
	// instancia un nuevo dialogo
	wxDialog *deleteDialog = new wxDialog(this, wxID_ANY, "Delete item?", wxDefaultPosition, wxSize(700, 300));
	// crea sizer para los botones
	wxSizer *buttonSizer = deleteDialog->CreateButtonSizer(wxYES | wxCANCEL);
	// crea sizer para cuadros de texto que mustran ID y nombre
	wxGridSizer *gridSizer = new wxGridSizer(2, wxSize(10, 10));
	// crea los cuadros de texto y los agrega a su sizer
	wxTextCtrl *idBox = new wxTextCtrl(deleteDialog, wxID_ANY, getSelectedItemID(), wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_CENTER);
	wxTextCtrl *nameBox = new wxTextCtrl(deleteDialog, wxID_ANY, getSelectedItemName(), wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_CENTER);
	gridSizer->Add(idBox, 1, wxEXPAND);
	gridSizer->Add(nameBox, 1, wxEXPAND);
	// sizer principal, se ajusta al dialogo
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 5);
	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
	deleteDialog->SetSizerAndFit(mainSizer);
	
	// conecta el evento del boton "Yes"
	deleteDialog->Bind(wxEVT_BUTTON, 
						[this, &deleteDialog](wxCommandEvent &evt) {	

							uint itemID = wxAtoi(this->getSelectedItemID());
							if (_findItem(head, itemID) == NULL) {
								ErrorBox::callError(ERR_NOT_FOUND);
								printf("Item not found\n");
								deleteDialog->Destroy();
								return;
							}
							// delete item from linked list
							deleteItem(&head, itemID);
							// delete item from listctrl and close dialog
							mainListView->DeleteItem(this->getSelectedItemIndex());
							deleteDialog->Destroy();
						},
						wxID_YES);
	// muestra el dialogo
	deleteDialog->ShowModal();
	deleteDialog->Destroy();
}
void MainFrame::onClose(wxCloseEvent &evt)
{
	// al cerrar la ventana se borra la lista de la memoria
	mainListView->freeList();
	// cierra la ventana
	Destroy();
}

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_BUTTON(wxID_ADD, MainFrame::onAddItemButton)
	EVT_BUTTON(wxID_APPLY, MainFrame::onApplyMovementButton)
	EVT_BUTTON(wxID_SAVE, MainFrame::onSaveButton)
	EVT_BUTTON(wxID_DELETE, MainFrame::onDeleteButton)
	EVT_CLOSE(MainFrame::onClose)
wxEND_EVENT_TABLE()

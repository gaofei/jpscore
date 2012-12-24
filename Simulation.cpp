

#include "Simulation.h"

/************************************************
// Konstruktoren
 ************************************************/

Simulation::Simulation() {
	pOnline = false; // true when online simulation (TraVisTo) is wished
	pLinkedCells = false;
	pActionPt = 0; // on or off, wird für GCFM benötigt
	pNPeds = 0; // number of pedestrians, Default 10
	pTmax = 0;
	pDt = 0;
	pBuilding = NULL;
	pDistribution = NULL;
	pDirection = NULL;
	pModel = NULL;
	pSolver = NULL;
	iod = new IODispatcher();
	pTrajectories = new Trajectories();
	fps=1;
}

Simulation::~Simulation() {
	delete pBuilding;
	delete pDistribution;
	delete pDirection;
	delete pModel;
	delete pSolver;
	delete iod;
	delete pTrajectories;
}

/************************************************
// Setter-Funktionen
 ************************************************/

int Simulation::SetOnline(bool o) {
	return pOnline = o;
}

int Simulation::SetNPeds(int i) {
	return pNPeds = i;
}

int Simulation::SetLinkedCells(bool l) {
	return pLinkedCells = l;
}

/************************************************
// Getter-Funktionen
 ************************************************/

bool Simulation::IsOnline() const {
	return pOnline;
}

bool Simulation::IsLinkedCells() {
	return pLinkedCells;
}

int Simulation::GetNPeds() const {
	return pNPeds;
}

Building * Simulation::GetBuilding() const {
	return pBuilding;
}

/************************************************
// Sonstige-Funktionen
 ************************************************/

/* bekommt alle Konsolenoptionen vom ArgumentParser
 * und setzt die entsprechenden Parameter in der Simulation
 * */
void Simulation::InitArgs(ArgumentParser* args) {
	char tmp[CLENGTH];
	string s = "Parameter:\n";

	switch (args->GetLog()) {
		case 0:
			// no log file
			//Log = new OutputHandler();
			break;
		case 1:
			if(Log) delete Log;
			Log = new STDIOHandler();
			break;
		case 2:
		{
			char name[CLENGTH]="";
			sprintf(name,"%s.P0.dat",args->GetErrorLogFile().c_str());
			if(Log) delete Log;
			Log = new FileHandler(name);
		}
		break;
		default:
			printf("Wrong option for Logfile!\n\n");
			exit(0);
	}

	Log->write("INFO: \tOptionen an Simulation geben\n");

	// Online Simulation for TraVisTo?
	switch (args->GetTravisto()) {
		case 0:
			break;
		case 1:
		{
			//iod = new IODispatcher();
			OutputHandler* tofile = new FileHandler(args->GetTrajOutputDir().c_str());
			iod->AddIO(tofile);
			break;
		}

		case 2:
		{
			OutputHandler* tofile = new FileHandler(args->GetTrajOutputDir().c_str());
			iod->AddIO(tofile);
			OutputHandler* travisto = new TraVisToHandler();
			iod->AddIO(travisto);
			pOnline = true;
			break;
		}
		case 3: //plain text
			delete iod; // delete the previously allocated memory
			Log->write("ERROR:\tdon't use this writer, not implemented");
			exit(0);
			break;

		case 4: //flat format
		{
			OutputHandler* file = new FileHandler("./Trajektorien.dat");
			pTrajectories->AddIO(file);
		}
			break;

		default:
			printf("Wrong option for TraVisTo Output!\n\n");
			exit(0);
	}


	if (pOnline)
		s.append("\tonline\n");

	// Linked Cells?
	pLinkedCells = args->GetLinkedCells();
	if (pLinkedCells)
		s.append("\tusing Linked-Cells\n");

	pDistribution = new PedDistributor(args->GetV0Mu(), args->GetV0Sigma(), args->GetBmaxMu(),
			args->GetBmaxSigma(), args->GetBminMu(), args->GetBminSigma(), args->GetAtauMu(),
			args->GetAtauSigma(), args->GetAminMu(), args->GetAminSigma(), args->GetTauMu(),
			args->GetTauSigma());
	s.append(pDistribution->writeParameter());
	pDistribution->InitDistributor(args->GetPersonsFilename());

	// Richtungswahl zum Ziel
	int direction = args->GetExitStrategy();
	sprintf(tmp, "\tRichtung zum Ausgang: %d\n", direction);
	s.append(tmp);
	switch (direction) {
		case 1:
			pDirection = new DirectionMiddlePoint();
			break;
		case 2:
			pDirection = new DirectionMinSeperation();
			break;
		case 3:
			pDirection = new DirectionMinSeperationShorterLine();
			break;
		case 4:
			pDirection = new DirectionInRangeBottleneck();
			break;
	}
	pModel = new GCFMModel(pDirection, args->GetNuPed(), args->GetNuWall(), args->GetDistEffMaxPed(),
			args->GetDistEffMaxWall(), args->GetIntPWidthPed(), args->GetIntPWidthWall(),
			args->GetMaxFPed(), args->GetMaxFWall());
	s.append("\tModel: GCFMModel\n");
	s.append(pModel->writeParameter());

	// ODE Löser
	int solver = args->GetSolver();
	sprintf(tmp, "\tODE Loeser: %d\n", solver);
	s.append(tmp);
	switch (solver) {
		case 1:
			if (pLinkedCells)
				pSolver = new EulerSolverLC(pModel);
			else
				pSolver = new EulerSolver(pModel);
			break;
		case 2:
			pSolver = new VelocityVerletSolver(pModel);
			break;
		case 3:
			pSolver = new LeapfrogSolver(pModel);
			break;
	}
	pTmax = args->GetTmax();
	sprintf(tmp, "\tt_max: %f\n", pTmax);
	s.append(tmp);
	pDt = args->Getdt();
	sprintf(tmp, "\tdt: %f\n", pDt);
	s.append(tmp);

	fps=args->Getfps();
	sprintf(tmp, "\tfps: %f\n", fps);
	s.append(tmp);

	// Routing
	int router = args->GetRoutingStrategy();
	sprintf(tmp, "\tRouting Strategy: %d\n", router);
	s.append(tmp);
	Routing* rout = NULL;
	switch (router) {
		case 1:
			rout = new GlobalRouter();
			break;
		case 2:
			rout = new GlobalRouter();
			break;
		case 3:
			rout = new QuickestPathRouter();
			break;
		case 4:
		    rout = new GraphRouter();
			break;
		case 5:
			cout<<"router not available"<<endl;
			exit(EXIT_FAILURE);
			break;
		case 6:
			rout = new DummyRouter();
			break;
		default:
			cout<<"router not available"<<endl;
			exit(EXIT_FAILURE);
			break;
	}

	// IMPORTANT: do not change the order in the following..
	pBuilding = new Building();
	pBuilding->SetRouting(rout);

	sprintf(tmp, "\tGeometrie: [%s]\n", args->GetGeometryFilename().c_str());
	s.append(tmp);
	Log->write("INFO: \t" + s);
	pBuilding->LoadBuilding(args->GetGeometryFilename());
	//pBuilding->AddSurroundingRoom();
	pBuilding->InitGeometry(); // create the polygones

	pBuilding->LoadTrafficInfo(args->GetTrafficFile());

	pBuilding->LoadRoutingInfo(args->GetRoutingFile());

	/////////

	//Sean
	//NavMesh* nv= new NavMesh(pBuilding);
	//nv->BuildNavMesh();
	//nv->WriteToFile("../pedunc/examples/stadium/arena.nav");
	//nv->WriteToFileTraVisTo("promenade.nav.xml");
	//exit(EXIT_FAILURE);

	//iod->WriteGeometryRVO(pBuilding);exit(EXIT_FAILURE);
	//iod->WriteNavMeshORCA(pBuilding);exit(EXIT_FAILURE);


	////////


	pNPeds=pDistribution->Distribute(pBuilding);

	// initialise the routing engine before doing any other things
	rout->Init(pBuilding);
	pBuilding->InitPhiAllPeds(pDt);

	//using linkedcells
	if (pLinkedCells){
		pBuilding->InitGrid(args->GetLinkedCellSize());
	}

	// init pathway
	if(args->GetPathwayFile()!=""){
		char name[30]="";
		sprintf(name,"%s_p0",args->GetPathwayFile().c_str());
		pBuilding->InitSavePedPathway(name);
	}

	//pBuilding->WriteToErrorLog();
}


/* Eigentliche Simulation
 * Rückgabewert:
 *   - Evakuierungszeit
 * */

int Simulation::RunSimulation() {
	int frameNr = 1; // Frame Number
	int writeInterval = (int) ((1. / fps) / pDt + 0.5);
	writeInterval = (writeInterval <= 0) ? 1 : writeInterval; // mustn't be <= 0
	double t;


	// writing the header

	iod->WriteHeader(pNPeds, fps, pBuilding);
	iod->WriteGeometry(pBuilding);
	iod->WriteFrame(0, pBuilding);

	//first initialisation needed by the linked-cells
	if (pLinkedCells) Update();


	// main program loop
	for (t = 0; t < pTmax && GetNPeds() > 0; ++frameNr) {
		t = 0 + (frameNr - 1) * pDt;
		// solve ODE: berechnet Kräfte und setzt neue Werte für x und v
		pSolver->solveODE(t, t + pDt, pBuilding);
		// gucken ob Fußgänger in neuen Räumen/Unterräumen
		Update();
		// ggf. Ausgabe für TraVisTo
		if (frameNr % writeInterval == 0) {
			iod->WriteFrame(frameNr / writeInterval, pBuilding);
		}
//		if (frameNr % 1000 == 0) {
//			cout<<"                        \r";
//			cout<<">> Frames : "<<frameNr<<"\r";
//			cout.flush();
//		}
	}
	// writing the footer
	iod->WriteFooter();

	//return the evacuation time
	return (int) t;
}

/* Geht alle Räume, dann Unterräume durch um zugucken
 * ob Fußgänger von einem Raum in einen anderen gesetzt
 * werden müßen
 * */

void Simulation::Update() {
	pBuilding->Update();
	// Neue Anzahl von Fußgänger, falls jemand ganz raus geht
	SetNPeds(pBuilding->GetAllPedestrians().size());

	// update the general time
	Pedestrian::SetGlobalTime(Pedestrian::GetGlobalTime()+pDt);

	//update the cells position
	if (pLinkedCells){
		pBuilding->UpdateGrid();
	}

}

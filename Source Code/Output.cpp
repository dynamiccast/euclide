#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "Constantes.h"
#include "Erreur.h"
#include "Output.h"
#include "Partie.h"
#include "Timer.h"

/*************************************************************/

static const char *FichiersTexte[] = {
	"Euclide.txt",
	"Fran�ais.txt",
	"English.txt"
};

static char *Textes[MaxTextes];

static const char *ChiffresRomains[] = {
	"",
	"",
	" II",
	" III",
	" IV",
	" V",
	" VI",
	" VII",
	" VIII",
	" IX"
};

/*************************************************************/

void ChoixDeLangue(langue Langue)
{
	if (Langue >= MaxLangues)
		Langue = EUCLIDE;

	for (unsigned int i = 0; i < MaxTextes; i++)
		Textes[i] = NULL;

	FILE *Source = fopen(FichiersTexte[Langue], "r");
	if (!Source)
		ErreurFichierLangue(FichiersTexte[Langue]);

	unsigned int TextesLus = 0;
	char Tampon[1024];
	while (fgets(Tampon, 1024, Source)) {
		unsigned int Length = strlen(Tampon);
		while ((Length > 0) && isspace(Tampon[Length - 1]))
			Tampon[--Length] = '\0';
			
		Textes[TextesLus] = new char[Length + 1];
		strcpy(Textes[TextesLus], Tampon);

		if (++TextesLus >= MaxTextes)
			break;
	}
	
	if (TextesLus < MaxTextes)
		ErreurFichierLangue(FichiersTexte[Langue]);

	fclose(Source);
}

/*************************************************************/

void DestructionDesTextes()
{
	for (unsigned int i = 0; i < MaxTextes; i++)
		delete[] Textes[i];
}

/*************************************************************/

const char *GetTexte(texte Texte, unsigned int LongueurMaximale, bool TailleExacte)
{
	static char Tampon[1024];

	unsigned int Longueur = strlen(Textes[Texte]);
	
	if (LongueurMaximale > 1023)
		LongueurMaximale = 1023;
	
	if (Longueur > LongueurMaximale)
		Longueur = LongueurMaximale;

	memcpy(Tampon, Textes[Texte], Longueur);
	
	if (TailleExacte) {
		memset(&Tampon[Longueur], ' ', LongueurMaximale - Longueur);
		Tampon[LongueurMaximale] = '\0';
	}
	else {
		Tampon[Longueur] = '\0';
	}

	return Tampon;
}

/*************************************************************/

char HommeToChar(hommes Homme)
{
	switch (Homme) {
		case XROI :
			return PieceToChar(ROI);
		case XDAME :
			return PieceToChar(DAME);
		case TOURDAME :
		case TOURROI :
			return PieceToChar(TOUR);
		case FOUDAME :
		case FOUROI :
			return PieceToChar(FOUBLANC);
		case CAVALIERDAME :
		case CAVALIERROI :
			return PieceToChar(CAVALIER);
		default :
			break;
	}

	if (Homme <= PIONH)
		return PieceToChar(PION);

	return '?';
}

/*************************************************************/

char PieceToChar(pieces Piece)
{
	static char Symboles[MaxPieces];
	static char Lire = true;

	if (Lire) {
		memcpy(Symboles, GetTexte(MESSAGE_SYMBOLES, MaxPieces, true), MaxPieces);
		Lire = false;
	}

	return (Piece < MaxPieces) ? Symboles[Piece] : '?';
}

/*************************************************************/

char ColonneToChar(colonnes Colonne)
{
	if (Colonne >= MaxColonnes)
		return '?';

	return (char)('a' + Colonne);
}

/*************************************************************/

char RangeeToChar(rangees Rangee)
{
	if (Rangee >= MaxRangees)
		return '?';

	return (char)('1' + Rangee);
}

/*************************************************************/

const char *CaseToString(cases Case)
{
	static char Tampon[4];

	Tampon[0] = (Case < MaxCases) ? ColonneToChar(QuelleColonne(Case)) : '?';
	Tampon[1] = (Case < MaxCases) ? RangeeToChar(QuelleRangee(Case)) : '?';
	Tampon[2] = '\0';

	return Tampon;
}

/*************************************************************/

void OutputMessageErreur(texte Message)
{
	OutputMessageErreur(GetTexte(Message, 256, false));
	OutputChrono(GetElapsedTime());
}

/*************************************************************/

void OutputNombreSolutions(unsigned int NombreSolutions, bool Duals)
{
	if (Duals) {
		OutputResultat(GetTexte(MESSAGE_COOKED, 32, false));
	}
	else if (NombreSolutions == 0) {
		OutputResultat(GetTexte(MESSAGE_ZEROSOLUTION, 32, false));
	}
	else if (NombreSolutions == 1) {
		OutputResultat(GetTexte(MESSAGE_UNESOLUTION, 32, false));
	}
	else {
		Verifier(NombreSolutions < 10000);

		char Texte[32];
		sprintf(Texte, "%u %s", NombreSolutions, GetTexte(MESSAGE_NSOLUTIONS, 27, false));
		OutputResultat(Texte);
	}

	OutputChrono(GetElapsedTime());
}

/*************************************************************/

void OutputMessage(texte Message, unsigned int Compte)
{
	Verifier(Compte < 10);

	char Tampon[1024];
	sprintf(Tampon, "%s%s...", GetTexte(Message, 256, false), ChiffresRomains[Compte]);

	OutputMessage(Tampon);
	OutputChrono(GetElapsedTime());
}

/*************************************************************/

void OutputSolution(const solution *Solution, unsigned int Numero)
{
	static FILE *Output = NULL;
	if (!Output)
		Output = fopen("Output.txt", "w");

	if (!Output)
		return;

	if (Numero == 1)
		fprintf(Output, "\n********************************************************************************\n\n");

	fprintf(Output, "%s #%u :\n", GetTexte(MESSAGE_SOLUTION, 64, false), Numero);
	fprintf(Output, "---------------------------------------------------------------------\n");

	for (unsigned int k = 0; k < Solution->DemiCoups; k++) {
		const deplacement *Deplacement = &Solution->Deplacements[k];

		if ((k % 2) == 0)
			fprintf(Output, "%2u. ", (k / 2) + 1);

		if (Deplacement->Roque) {
			if (QuelleColonne(Deplacement->Vers) == C)
				fprintf(Output, "0-0-0    ");
			else
				fprintf(Output, "0-0      ");
		}
		else {
			fprintf(Output, "%c%s%c%c%c%c%c ", Deplacement->Promotion ? HommeToChar(Deplacement->Qui) : PieceToChar(Deplacement->TypePiece), CaseToString(Deplacement->De), (Deplacement->Mort == MaxHommes) ? '-' : 'x', ColonneToChar(QuelleColonne(Deplacement->Vers)), RangeeToChar(QuelleRangee(Deplacement->Vers)), Deplacement->Promotion ? '=' : (Deplacement->EnPassant ? 'e' : ' '), Deplacement->Promotion ? PieceToChar(Deplacement->TypePiece) : (Deplacement->EnPassant ? 'p' : ' '));
		}

		if ((k % 2) == 1)
			fprintf(Output, " ");

		if ((k % 6) == 5)
			if (k != (Solution->DemiCoups - 1))
				fprintf(Output, "\n");
	}

	fprintf(Output, "\n\n");
	fflush(Output);
}

/*************************************************************/
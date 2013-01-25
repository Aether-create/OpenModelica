// ObjectBase.h: Schnittstelle f�r die Klasse IObject.
//
//////////////////////////////////////////////////////////////////////
 
#pragma once


typedef std::string UIDSTR;


//! Zur Unterscheidung unterschiedlicher Berechnungsaufrufe in updateOutput bzw. writeOutput
enum OUTPUTTYPE
{
    UNDEF_OUTPUT    =    0x00000000,

    WRITE            =    0x00000001,            ///< aktuelle Curser-Position speichern und Ausgabe schreiben
    RESET            =    0x00000002,            ///< Curser auf letzte g�ltige Position im File setzten
    OVERWRITE        =    0x00000003,            ///< RESET|WRITE (Curser zur�cksetzen, Curserpositon neu speichern und Ausgabe schreiben)

    HEAD_LINE        =    0x00000010,            ///< Ausgabe der �berschriftenzeile passend zu den Ergebnissen
    RESULTS            =    0x00000020,            ///< Ausgabe der Ergebnisse
    PROPERTIES        =    0x00000040,            ///< Ausgabe der Eigenschaften
    INITIALIZE_OUTPUT=    0x00000100,            ///< Initialize FAMOS output
    FINALIZE_OUTPUT =    0x00000200            ///< Complete FAMOS output
};

class IObject
{
public:

    virtual ~IObject()    {};

    virtual void destroy() = 0;

    virtual UIDSTR getUID() const = 0 ;

    virtual std::string getName() const    = 0;

    virtual void setName(const std::string name) = 0;


    /// (Re-)Initialisiert das Objekt und schlie�t den Aufbau ab
    virtual void init()    = 0;

    
};




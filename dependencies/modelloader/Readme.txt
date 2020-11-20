Wavefront Object (*.obj) Model-Loader 
Autor: Andreas Klein (klein.andreas@gmail.com)

Getestet unter: Windows Vista SP1 mit Visual Studio 2008, Ubuntu Linux 8.10 mit g++ 4.3
Benötigte Bibliotheken: C++-Standardbibliothek


Aus einer Datei werden nur die Dreiecke und Eckpunkte geladen, alles andere wird ignoriert. Es
wir davon ausgegangen, dass das Model eine Dreieckstesselierung besitzt. Eigene Modelle müssen
entsprechend tesseliert werden (z.B. mit Blender oder Wings3D). 

Der Model-Loader besteht aus den Klassen und Strukturen: Model, Triangle, Vertex und Vector3.
Sämtliche Klassen sind im Namespace "obj" abgelegt.

Mit Ausnahme von Vector3, müssen die Strukturen und Klassen nicht weiter angepasst werden. In der 
Vertex-und Triangle-Struktur ist jeweils ein Normalenvektor definiert, der entsprechend berechnet 
werden muss. Über die Methode GetAdjacentTriangles der Model-Klasse können die angrenzenden Dreiecke 
abgefragt werden. 

Beispiel zum Laden eines Modells:

Laden eines Models:
Model model("bunny.obj");

oder alternativ:

Model model;
model.Load("bunny.obj");



Die bereitgestellten Modelle wurden normiert und in den Koordinatenursprung gesetzt.
Des weiteren ist folgendes Copyright zu beachten:
 
Bunny.obj 	 Copyright Stanford University http://graphics.stanford.edu/data/3Dscanrep/
Killeroo.obj 	 Copyright headus / Rezard http://www.headus.com/au/samples/killeroo/index.html
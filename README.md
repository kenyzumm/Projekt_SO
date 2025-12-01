#ZADANIE PROJEKTOWE
Opracować zestaw programów typu producent - konsument realizujących przy wykorzystaniu mechanizmu kolejek komunikatów (między procesami P1-P2) oraz mechanizmu semaforów i pamięci dzielonej (między procesami P2-P3), następujący schemat komunikacji międzyprocesowej:

Proces 3:
czyta dane (pojedyncze wiersze) ze standardowego strumienia wejściowego lub pliku i przekazuje je w niezmienionej formie do procesu 2.

Proces 2:
pobiera dane przesłane przez proces 3. Oblicza ilość znaków w każdej linii i wyznaczoną liczbę przekazuje do procesu 1.

Proces 1:
pobiera dane wyprodukowane przez proces 2 i umieszcza je w standardowym strumieniu wyjściowym. Każda odebrana jednostka danych powinna zostać wyprowadzona w osobnym wierszu.

Należy zaproponować i zaimplementować mechanizm informowania się procesów o swoim stanie. Należy wykorzystać do tego dostępny mechanizm sygnałów i łączy komunikacyjnych (pipes). Scenariusz powiadamiania się procesów o swoim stanie wygląda następująco: do procesu 2 wysyłane są sygnały. Proces 2 przesyła otrzymany sygnał do procesu macierzystego. Proces macierzysty zapisuje wartość sygnału do łączy komunikacyjnych oraz wysyła powiadomienie do procesu 3 o odczytanie zawartości łącza komunikacyjnego. Proces 3 po odczytaniu sygnału wysyła powiadomienie do procesu 2 o odczytanie łącza komunikacyjnego. Proces 2 powiadamia proces 1 o konieczności odczytu łącza komunikacyjnego. Wszystkie trzy procesy powinny być powoływane automatycznie z jednego procesu inicjującego.

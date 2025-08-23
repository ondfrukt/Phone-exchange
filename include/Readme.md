## MCPDriver.cpp ##

MCPDriver är en klass som håller alla funktioner som behövs för att MCP-enheterna ska fungera och kunna utföra sina mest basala uppgifter såsom att läsa värden på GPIO-portar. Ha funktioner för att stänga på och av specifika portar. Generera en INT-signal om vissa av portarna ändrar värde. Klassen ska även kunna på förfrågan leverara vilken gpio-pin på mcp:m som orsakade INT-signalen och dess nya värde.

Biblioteket som ska användas är Adafruit_MCP23X17. mitt bibliotek har inte funktionen getLastInterruptPinValue () så detta måste lösas på annat sätt.

Wire aktiveras på annat håll och behöver inte has med i klassen utan kan betraktas som igångsatt innan

### Egenskaper ###
 - Hitta och retunera true om MCP-enheterna hittades på I2C - nätet. Retunerar även vilken MCP som inte lyckades.
 - Starta begin_I2C för respektive MCP enhet med hjälp av adresserna från Config.h
 - Ställa in egenskaperna för alla GPIO-portar på MCP-enheterna där config.h innehåller inställningarna.
 - Ställa in rätt INT-egenskaper som där INTA och INTB är ihopkopplade och ska vara open drain.
 - Ställa in vilka GPIO-pinnar som ska aktivera INT. Detta är alla SHK-pinnar och INT ska ge signal vid CHANGE.


 - Ställa in min ESP32:s GPIO-inställningar för INT-ingångar. Dessa finns angivna i config.h
 - Ställa in interrupt på dessa INT-ingångar. Interrupt-funktionen ska enbart sätta upp en flagga.
 - Funktion reagerar om flaggan = true och som sedan retunerar vilken vilken GPIO-pinne på MCPn som genererade INT-signalen och vilket värde den har och kvitterar MCPns i 
 - Kvittering av INT-meddelandet i lämplig funktion.


 ## SHKDebounce.cpp ##

 SHKDebouncing är en klass som hanterar debouncing av SHK-pinnar på MCU-gpio. klassen ska ta emot värden från en GPIO som har ändrats och lagra detta genom att 
 
 ## LineHandler ##

 LineHandler är en klass som skapar Line-objekt. Dessa objekt håller respektive linjes egenskaper såsom, telefonnummer, om luren är på eller av, siffror som slagits in, linjens status osv.

 ## LineManager ##
 LineManager är en klass som håller koll på alla LineHandler-objekt och utför metoder på dessa. Det exempelvis objektet av denna klass som scannar igenom alla linjer för att se om något har förändrats på något utav dem och vid behov initierar rätt metoder.

 ## Settings ##
 Settings håller och tillhandahåller alla golbala variabler och sparar även dessa, även om ESP:n skulle starta om.
TP Criptografia y Seguridad 2015

Para compilar el proyecto, se requiere contar con gcc y make.  Ejecutar 'make' en la carpeta raiz
del proyecto para generar el binario 'cripto' en el directorio 'bin'.

Para utilizar el proyecto, seguir las instrucciones de parametros del enunciado.  Se agregaron dos
parametros adicionales:

--no-permute: desactiva la permutacion al recuperar o distribuir
--verbose: activa el modo 'verbose', que imprime informacion util en pantalla

Al utilizar el modo -r (recuperacion), si k = 8, no se requiere especificar el ancho y alto de
la imagen secreta a recuperar (se infiere de las sombras).  Si k != 8, es necesario especificar el
ancho y el alto de la imagen original utilizando los parametros -w WIDTH y -h HEIGHT.  Consultar el
informe para determinar el tamaño necesario de sombra para una imagen secreta.

El header BMP de la imagen recuperada se copia de la primera sombra detectada, en orden alfabetico
(y se actualizan los campos relevantes).
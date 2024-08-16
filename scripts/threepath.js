const fs = require('fs');
const path = require('path');

// Función para obtener la estructura de directorios de manera recursiva
function getDirectoryStructure(dirPath) {
  const structure = {};

  const items = fs.readdirSync(dirPath);

  items.forEach(item => {
    const itemPath = path.join(dirPath, item);
    const stats = fs.statSync(itemPath);

    if (stats.isDirectory()) {
      structure[item] = getDirectoryStructure(itemPath);
    } else {
      structure[item] = 'file';
    }
  });

  return structure;
}

// Guardar la estructura de directorios en un archivo
function saveDirectoryStructureToFile(dirPath, outputFile) {
  const directoryStructure = getDirectoryStructure(dirPath);
  fs.writeFileSync(outputFile, JSON.stringify(directoryStructure, null, 2), 'utf-8');
  console.log(`Estructura de directorios guardada en ${outputFile}`);
}

// Ejecución
const dirPath = process.cwd(); // Ruta del directorio desde donde se ejecuta el script
const outputFile = path.join(dirPath, 'estructura_directorios.json'); // Archivo de salida

saveDirectoryStructureToFile(dirPath, outputFile);

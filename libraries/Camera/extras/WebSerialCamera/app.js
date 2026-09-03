/**
 * @fileoverview This file contains the main application logic.
 * 
 * The application uses the Web Serial API to connect to the serial port.
 * Check the following links for more information on the Web Serial API:
 * https://developer.chrome.com/articles/serial/
 * https://wicg.github.io/serial/
 * 
 * The flow of the application is as follows:
 * 1. The user clicks the "Connect" button or the browser automatically connects 
 * to the serial port if it has been previously connected.
 * 2. The application requests the camera configuration (mode and resolution) from the board.
 * 3. The application starts reading the image data stream from the serial port. 
 * It waits until the expected amount of bytes have been read and then processes the data.
 * 4. The processed image data is rendered on the canvas.
 * 
 * @author Sebastian Romero
 */

const connectButton = document.getElementById('connect');
const refreshButton = document.getElementById('refresh');
const startButton = document.getElementById('start');
const saveImageButton = document.getElementById('save-image');
const filterSelector = document.getElementById('filter-selector');
const resolutionSelector = document.getElementById('resolution-selector');
const cameraModeSelector = document.getElementById('camera-mode-selector');
const canvas = document.getElementById('bitmapCanvas');
const ctx = canvas.getContext('2d');

const imageDataTransfomer = new ImageDataTransformer(ctx);
const connectionHandler = new SerialConnectionHandler();

function applyCameraSettingsFromSelectors() {
  const config = {
    resolutionKey: resolutionSelector.value,
    modeKey: cameraModeSelector.value
  };

  persistCameraConfig(config);

  const resolvedConfig = getCameraConfig();
  imageDataTransfomer.setImageMode(resolvedConfig.mode);
  imageDataTransfomer.setResolution(resolvedConfig.width, resolvedConfig.height);

  if (resolvedConfig.mode !== 'GRAYSCALE') {
    filterSelector.disabled = false;
  } else {
    filterSelector.disabled = true;
    filterSelector.value = 'none';
    imageDataTransfomer.filter = null;
  }

  console.log(`📷 Camera configuration set: ${resolvedConfig.mode} @ ${resolvedConfig.width}x${resolvedConfig.height}`);
}

function initializeCameraSelectors() {
  const storedConfig = getStoredCameraConfig();
  resolutionSelector.value = storedConfig.resolutionKey || DEFAULT_CAMERA_CONFIG.resolutionKey;
  cameraModeSelector.value = storedConfig.modeKey || DEFAULT_CAMERA_CONFIG.modeKey;
  applyCameraSettingsFromSelectors();
}

// Connection handler event listeners

connectionHandler.onConnect = async () => {
  connectButton.textContent = 'Disconnect';

  const config = getCameraConfig();
  console.log(`📷 Applied camera configuration: ${config.mode} @ ${config.width}x${config.height}`);

  imageDataTransfomer.setImageMode(config.mode);
  imageDataTransfomer.setResolution(config.width, config.height);

  if (config.mode !== 'GRAYSCALE') {
    filterSelector.disabled = false;
  } else {
    filterSelector.disabled = true;
    filterSelector.value = 'none';
    imageDataTransfomer.filter = null;
  }

  renderStream();
};

connectionHandler.onDisconnect = () => {
  imageDataTransfomer.reset();
  connectButton.textContent = 'Connect';
  filterSelector.disabled = true;
  filterSelector.value = 'none';
};


// Rendering logic

async function renderStream(){
  while(connectionHandler.isConnected()){
    if(imageDataTransfomer.isConfigured()) await renderFrame();
  }
}

/**
 * Renders the image data for one frame from the board and renders it.
 * @returns {Promise<boolean>} True if a frame was rendered, false otherwise.
 */
async function renderFrame(){
  if(!connectionHandler.isConnected()) return;
  console.log('[WebSerialDebug] requesting a new frame from the board...');
  const imageData = await connectionHandler.getFrame(imageDataTransfomer);
  if(!imageData) {
    console.log('[WebSerialDebug] no frame returned from the board');
    return false; // Nothing to render
  }
  console.log('[WebSerialDebug] frame received:', imageData.width, 'x', imageData.height);
  if(!(imageData instanceof ImageData)) throw new Error('🚫 Image data is not of type ImageData'); 
  renderBitmap(ctx, imageData);
  return true;
}

/**
 * Renders the image data on the canvas.
 * @param {CanvasRenderingContext2D} context The canvas context to render on.
 * @param {ImageData} imageData The image data to render.
 */
function renderBitmap(context, imageData) {
  context.canvas.width = imageData.width;
  context.canvas.height = imageData.height;
  context.clearRect(0, 0, canvas.width, canvas.height);
  context.putImageData(imageData, 0, 0);
}


// UI Event listeners

startButton.addEventListener('click', renderStream);

connectButton.addEventListener('click', async () => { 
  if(connectionHandler.isConnected()){
    connectionHandler.disconnectSerial();
  } else {
    await connectionHandler.requestSerialPort();
    await connectionHandler.connectSerial();
  }
});

refreshButton.addEventListener('click', () => {
  if(imageDataTransfomer.isConfigured()) renderFrame();
});

saveImageButton.addEventListener('click', () => {
  const link = document.createElement('a');
  link.download = 'image.png';
  link.href = canvas.toDataURL();
  link.click();
  link.remove();
});

filterSelector.addEventListener('change', () => {
  const filter = filterSelector.value;
  switch(filter){
    case 'none':
      imageDataTransfomer.filter = null;
      break;
    case 'gray-scale':
      imageDataTransfomer.filter = new GrayScaleFilter();
      break;
    case 'black-and-white':
      imageDataTransfomer.filter = new BlackAndWhiteFilter();
      break;
    case 'sepia':
      imageDataTransfomer.filter = new SepiaColorFilter();
      break;
    case 'pixelate':
      imageDataTransfomer.filter = new PixelateFilter(8);
      break;
    case 'blur':
      imageDataTransfomer.filter = new BlurFilter(8);
      break;
    default:
      imageDataTransfomer.filter = null;
  }
});

resolutionSelector.addEventListener('change', () => {
  persistCameraConfig({
    resolutionKey: resolutionSelector.value,
    modeKey: cameraModeSelector.value
  });
  applyCameraSettingsFromSelectors();
});

cameraModeSelector.addEventListener('change', () => {
  persistCameraConfig({
    resolutionKey: resolutionSelector.value,
    modeKey: cameraModeSelector.value
  });
  applyCameraSettingsFromSelectors();
});

// On page load event, try to connect to the serial port
window.addEventListener('load', async () => {
  initializeCameraSelectors();
  filterSelector.disabled = true;
  console.log('🚀 Page loaded. Trying to connect to serial port...');
  
  setTimeout(() => {
    connectionHandler.autoConnect();
  }, 1000);
});

if (!("serial" in navigator)) {
  alert("The Web Serial API is not supported in your browser.");
}
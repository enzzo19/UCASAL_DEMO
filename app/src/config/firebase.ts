import { initializeApp } from 'firebase/app';
import { getDatabase, ref, onValue, get } from 'firebase/database';

const firebaseConfig = {
  apiKey: "AIzaSyCRQ6tFUCTiMN4L_ulNRTDMSdJhVmKcvog",
  databaseURL: "https://ucasal-demo-iot-default-rtdb.firebaseio.com",
};

const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

export { database, ref, onValue, get };

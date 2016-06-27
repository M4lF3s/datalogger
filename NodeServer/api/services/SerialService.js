/**
 * SerialService
 *
 * @author      :: Marcel Fraas
 * @description :: Server-side logic for managing serials
 */

var serialport = require("serialport");
var SerialPort = serialport.SerialPort;

module.exports = {

  // Implementation of the open-function for the SerialController
  open: function (connection) {
    var port = new SerialPort(connection.name, {
      baudrate: connection.baudrate,
      parser: serialport.parsers.readline('\n')
    }, true, function () {
      t = Math.floor(Date.now() / 1000);
      console.log("Port Opened...Current Time is: " + t);

      // Write the current Timestamp to the COM-Port
      port.write(t+"\n", function (err, bytesWritten) {
        if (err) {
          return console.log('Error: ', err.message);
        }
        console.log(bytesWritten, 'bytes written');
      });
    });

    // serialport callback
    // listens for data and writes it to the database
    port.on('data', function (data) {
      console.log('Data: ' + data);

      var obj = {};                             // create an empty Object
      var dataString = ''+data;                 // convert the Data to String
      var inputData = dataString.split(',');    // creates an Array by splitting the String

      if(inputData[1] !== undefined){

        dataSplit = inputData[1];
        dataSplit = dataSplit.substring(0, dataSplit.length-2) + "." + dataSplit.substring(dataSplit.length-2);

        obj['timestamp'] = inputData[0]+"000";    // insert timestamp data
        obj['data'] = dataSplit;                  // insert value data

        // Write the Data within obj to the Database
        Data.create(obj).exec(function createCB(err, created){
          if (err) {
            return console.log('Error: ', err.message);
          }
          i = created.id;
          d = created.data;
          t = created.timestamp;
          console.log('Created Data Entry: ' + t + ' ' + d);

          // Notify the Websockets
          Data.publishCreate({id: i, timestamp: t, data: d});
        });
      }
    });
  }
};

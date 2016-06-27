/**
 * DataController
 *
 * @author      :: Marcel Fraas
 * @description :: Server-side logic for managing data
 */

module.exports = {

  // This "watch" endpoint is for Websockets only
	watch: function (req, res) {
    Data.watch(req.socket);
    console.log("Client subscribed to " + req.socket.id);
  },

  // This generates some Test-data
  test: function (req, res) {
    Data.create({'data' : '0', 'timestamp' : '1465226439000'}).exec(function createCB(err, created){
      console.log('Created Data Entry: ' + created.timestamp + ' ' + created.data);
      Data.publishCreate({id: created.id, timestamp: created.timestamp, data: created.data});
    });
    Data.create({'data' : '15', 'timestamp' : '1465226739000'}).exec(function createCB(err, created){
      console.log('Created Data Entry: ' + created.timestamp + ' ' + created.data);
      Data.publishCreate({id: created.id, timestamp: created.timestamp, data: created.data});
    });
    Data.create({'data' : '32', 'timestamp' : '1465227039000'}).exec(function createCB(err, created){
      console.log('Created Data Entry: ' + created.timestamp + ' ' + created.data);
      Data.publishCreate({id: created.id, timestamp: created.timestamp, data: created.data});
    });
    return res.send(200);
  }

};


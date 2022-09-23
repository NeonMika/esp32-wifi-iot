const express = require('express');
const router = express.Router();

const { MongoClient } = require('mongodb');

const uri = 'mongodb+srv://USER:PW@DB-PATH/db?retryWrites=true&w=majority'; # TODO
const client = new MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true });
client.connect((err) => {
  if (err) throw err;

  router.get('/', (req, res, next) => {
    client.db('messages').collection('temp').find().toArray((err, temps) => {
      res.send(temps);
    });
  });

  router.get('/5mins', (req, res, next) => {
    /*
     * Requires the MongoDB Node.js Driver
     * https://mongodb.github.io/node-mongodb-native
     */

    const agg = [
      {
        $group: {
          _id: {
            day_and_hour: {
              $substr: [
                '$timestamp', 0, 13,
              ],
            },
            five_min_slot: {
              $toInt: {
                $divide: [
                  {
                    $toDouble: {
                      $substr: [
                        '$timestamp', 14, 2,
                      ],
                    },
                  },
                  5,
                ],
              },
            },
            topic: '$topic',
          },
          avg_temp: {
            $avg: {
              $toDouble: '$temp',
            },
          },
          sensor_nr: {
            $first: '$sensorNr',
          },
          mac_address: {
            $first: '$mac',
          },
        },
      },
    ];

    const coll = client.db('messages').collection('temp');
    coll.aggregate(agg)
      // .limit(+req.params.n)
      .toArray((temp_err, aggregated_temps) => {
        if (err) {
          console.err(`#temprouter: ${temp_err}`);
          res.send(temp_err);
        } else {
          res.send(aggregated_temps);
        }
      });
  });
});

module.exports = router;

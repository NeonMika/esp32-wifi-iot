const express = require('express');
const router = express.Router();

const { MongoClient } = require('mongodb');

const uri = process.env.IOT_DB;
if(!uri) {
    throw new Error("Environment variable IOT_DB is not set, has to contain the database connection string");
}
const client = new MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true });
client.connect((err) => {
  if (err) throw err;

  router.get('/', (req, res, next) => {
    const temp_from = req.query.from ? new Date(+req.query.from) : new Date(0)
    const temp_to = req.query.to ? new Date(+req.query.to) : new Date(8640000000000000)
    console.log(temp_from)
    console.log(temp_to)
    const t_start = new Date().getTime()
    client.db('messages').collection('temp').find({
      "timestamp" : {
        "$gte": temp_from,
        "$lte": temp_to
      }
    }).toArray((err, temps) => {
      const t_end = new Date().getTime()
      const query_duration = t_end - t_start
      res.send({
        query_duration,
        "temps": temps
      });
    });
  });

  router.get('/mins', (req, res, next) => {
    /*
     * Requires the MongoDB Node.js Driver
     * https://mongodb.github.io/node-mongodb-native
     */

    const temp_from = req.query.from ? new Date(+req.query.from) : new Date(0)
    const temp_to = req.query.to ? new Date(+req.query.to) : new Date(8640000000000000)
    const bin_minutes = req.query.bin_minutes ? +req.query.bin_minutes : 5
    console.log(temp_from)
    console.log(temp_to)

    const agg = [
      {
        $match: {
          "timestamp" : {
            "$gte": temp_from,
            "$lte": temp_to
          }
        }
      },
      {
        $group: {
          _id: {
            year: { $year: '$timestamp' },
            month: { $month: '$timestamp' },
            day: { $dayOfMonth: '$timestamp' },
            hour: { $hour: '$timestamp' },
            five_min_slot: {
              $toInt: {
                $divide: [
                  {
                    $minute: '$timestamp'
                  },
                  bin_minutes,
                ],
              },
            },
            topic: '$topic',
          },
          temp: {
            $avg: {
              $toDouble: '$temp',
            },
          },
          sensorNr: {
            $first: '$sensorNr',
          },
          mac: {
            $first: '$mac',
          },
          timestamp: {
            $first: '$timestamp',
          },
          room: {
            $first: '$room',
          }
        },
      },
    ];

    const coll = client.db('messages').collection('temp');
    const t_start = new Date().getTime()
    coll.aggregate(agg)
      .toArray((temp_err, aggregated_temps) => {
        const t_end = new Date().getTime()
        const query_duration = t_end - t_start
        if (err) {
          console.err(`#temprouter: ${temp_err}`);
          res.send(temp_err);
        } else {
          console.log(aggregated_temps)
          res.send({
            query_duration,
            "temps": aggregated_temps
          });
        }
      });
  });
});

module.exports = router;

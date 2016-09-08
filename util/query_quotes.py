import urllib2, urllib, json

# First we need to create the query in YML
baseurl = "http://query.yahooapis.com/v1/public/yql?"
yql_query = "select * from yahoo.finance.quotes where symbol in (\"YHOO\",\"AAPL\",\"GOOG\",\"MSFT\", \"PIH\", \"FLWS\", \"FCCY\", \"SRCE\", \"VNET\", \"TWOU\", \"JOBS\")"
env_string = "store://datatables.org/alltableswithkeys"

# Now, lets encode the URL
yql_url = baseurl + urllib.urlencode({'q':yql_query}) + "&format=json&" + urllib.urlencode({'env':env_string})

# Send request
result = urllib2.urlopen(yql_url).read()

# Parse the json result
data = json.loads(result)

#print json.dumps(data['query']['results'], indent=4, separators=(',', ':'))

# Now iterate over the array of quotes

for quote in data['query']['results']['quote']:
    print "%s#%s#%s#%s#%s#%s#%s#%s#%s#%s" % (quote['Symbol'], quote['LastTradePriceOnly'], quote['LastTradeDate'], quote['DaysLow'], quote['DaysHigh'], quote['ChangeinPercent'], quote['Bid'], quote['Ask'], quote['Volume'], quote['MarketCapitalization'])

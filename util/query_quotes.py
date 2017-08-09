import urllib2, urllib, json, sys

def getChunk(currentLoop, queryEvery, companiesNames):
    startAt = currentLoop * queryEvery

    chunk = ""
    first = 1
    for i in xrange(startAt, startAt+queryEvery):
        if first:
            chunk += "\"" + companiesNames[i] + "\""
            first = 0
        else:
            chunk += ", \"" + companiesNames[i] + "\""
    return chunk

def queryChunk(chunk):
# First we need to create the query in YML
    baseurl = "http://query.yahooapis.com/v1/public/yql?"
    yql_query = "select * from yahoo.finance.quotes where symbol in ("+chunk+")"
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

if __name__ == '__main__':
# Read the companies list file
    companiesFile = sys.argv[1]
    companiesNames = []
    file = open(companiesFile, "r") 
    for line in file: 
        columns = line.split('#');
        companiesNames.append(columns[0])

    numCompanies = len(companiesNames);
    queryEvery = 30

    # Only consider multiple of 30
    numberOfLoops  = numCompanies / queryEvery

    currentLoop = 0
    while currentLoop < numberOfLoops:
        chunk = getChunk(currentLoop, queryEvery, companiesNames)
        queryChunk(chunk)
        currentLoop += 1


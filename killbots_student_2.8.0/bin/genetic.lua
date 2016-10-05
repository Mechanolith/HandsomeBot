print("Killbots Tournament")
dofile("serialise.lua")

setArena("Large")

totalRounds=5
bots = getRobots()

b1 = "HandsomeBot"
b2 = "CheesecakeBot"
totalWins = {}
totalWins[b1]={wins=0,timeouts=0,draws=0, fired=0, hit=0}
totalWins[b2]={wins=0,timeouts=0,draws=0, fired=0, hit=0}
addRobot(b1)
addRobot(b2)

attrib = {1.0,1.0,1.0,1.0}

function mutate(a)
	local m = {a[1],a[2],a[3],a[4]}
	local r = math.random(4)
	m[r] = m[r] + (math.random()-0.5)
	if m[r] < 0.0 then
		m[r] = 0.0
	end
	return m
end

attribs={}

for generations = 1,100 do
	for mutations=1,20 do
		attribs[mutations]={mutate(attrib),0}
		wins1 = 0
		wins2 = 0
	for round=0,totalRounds-1 do
		
		setAttributes(1,attribs[mutations][1])
		startRound()
--		print("Main Loop")
		elapsed = 0
--		print("Round "..(round+1).." of "..b1.." vs "..b2.." starting")
		while (not isRoundOver()) and (elapsed < 240) do
			update()
			elapsed = elapsed + 1.0/60.0
		end

		a1=isRobotAlive(0)
		a2=isRobotAlive(1)
		
		if a1 and (not a2) then
			wins1 = wins1 + 1
			notifyRobotWon(0,true)
			notifyRobotWon(1,false)
		elseif (not a1) and a2 then
			wins2 = wins2 + 1
			notifyRobotWon(0,false)
			notifyRobotWon(1,true)
		elseif (not a1) and (not a2) then
			wins1 = wins1 + 1
			wins2 = wins2 + 1
			totalWins[b1].draws = totalWins[b1].draws + 1
			totalWins[b2].draws = totalWins[b2].draws + 1
			notifyRobotWon(0,true)
			notifyRobotWon(1,true)
			print("DRAW: "..tostring(a1).." "..tostring(a2).." "..elapsed)
		elseif elapsed >= 240 then
			totalWins[b1].timeouts = totalWins[b1].timeouts + 1
			totalWins[b2].timeouts = totalWins[b2].timeouts + 1
			notifyRobotWon(0,false)
			notifyRobotWon(1,false)
			print("TIMEOUT: "..tostring(a1).." "..tostring(a2).." "..elapsed)
		else
			print("ERROR: Both bots survived without timing out? "..tostring(a1).." "..tostring(a2).." "..elapsed)
		end
--		print("Time = "..elapsed)
	end
	print(b1.."="..wins1.."  "..b2.."="..wins2)
	attribs[mutations][2] = wins2
	end
	best = 1
	for m = 2,20 do
		if attribs[m][2]>attribs[best][2] then
			best = m
		end
	end
--	print(attribs[best][1])
	attrib = attribs[best][1]
	print("Attribs = "..attrib[1].." "..attrib[2].." "..attrib[3].." "..attrib[4])
end

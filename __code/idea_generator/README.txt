This is an easy to use idea generator for Hearts of Iron IV made by TheGamerCant

The idea behind it is simple - dynamic modifiers are easy to use but require checking variables every day - causing a lot of lag.
Ideas on the other hand are performance-light and far more flexible (can have rules, equipment modifiers, etc.) but get exponentially more complicated to code as the more complex your tree/decisions/events become.

That's where this program comes in - you input any transformations you want the idea to have and it generates all necessary ideas for you - so you only have to call a scripted effect to get the correct changes.
Here's how it works:

To set your idea, edit the idea.json file in any text editor of your choice.

It must have a string name at "idea_id", and can optionally have a localised name at "localised_name" and a gfx at "localised_name"

You then set the original base idea's modifiers under the "base_modifiers" branch - this should be fairly self-explanatory for anyone who's modded HoI4 before

After that, you define "transformations" - each transformation representing an effect from a focus, decision, event, etc. that changes the modifiers of the idea.
Each transformation needs a unique name at "transformation_name", as well as "prerequisite" and "mutually_exlusive" arguments.

Mutually exclusive arguments are fairly simple to understand - you enter a list of transformation names that this transformation cannot be taken with - such as mutually exclusive focuses or different event decisions

Prerequisite arguments are similar to focuses - this has to be entered as a two-dimensional list of strings, such as [["trns_1"], ["trns_2"], ["trns_3", "trns_4"]]
These arguments are similar to how a focus' prerequisite arguments' are written - an AND argument is applied for the first layer of the list, and an OR is applied for the second layer
So for the example given, our transformation can only occur if transformations 1 AND 2 have been taken, as well as transformation 3 OR 4



A few heads up:

All variables must be entered as decimal strings, otherwise the program will raise an exception
This is except for "instant = yes/no" arguments in equipment modifiers and rules - whatever value you put for them will overwrite the previous values
Such arguments might be a bit buggy in game however, if you're doing a lot of rule changes, you probably won't want to be doing them with this code - this code is best used as just an alternative to using dynamic modifiers

When coding dynamic modifiers, a strength is that you can edit them from just about anywhere - you can change the same modifiers' values from focuses, events and decisions without issue
Doing that here will probably generate an extremely large amount of ideas however - we need an idea for every possible state of the idea/modifier after all
Let's say you have an "army reforms" dynamic modifier that can be changed from both a focus tree and decisions - it might be a good idea to split it into two ideas (one gets modified from decisions, the other from the focus tree), and then merge it into one idea at the end with a "finish army reforms" decision that gives some attack bonuses or something
Otherwise, we'll have to generate an idea for every possible state of the focus tree, and then exponentially generate ideas to cope with every possible combination of decisions that can be taken - it can be done, but it would require a lot of ideas
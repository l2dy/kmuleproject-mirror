//this file is part of kMule
//Copyright (C)2012 WiZaRd ( strEmail.Format("%s@%s", "thewizardofdos", "gmail.com") / http://www.emulefuture.de )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#pragma warning(disable:4702) // unreachable code
#include <vector>
#pragma warning(default:4702) // unreachable code
#include <algorithm> //sort

template <class T> class CMedian
{
public:
	CMedian()	{}
	~CMedian()	{}

	void	AddVal(const T& val)
	{
		m_MedianValues.push_back(val);
	}

	void	Clear()
	{
		m_MedianValues.clear();
	}

	void	SetMedianVector(const std::vector<T> vec)
	{
		m_MedianValues = vec;
	}

	T		GetMedian()
	{
		std::vector<T>::size_type number = m_MedianValues.size();

		if(number == 0)
			return 0;
		if(number == 1) 
			return m_MedianValues.at(0);
		if(number == 2) 
			return (m_MedianValues.at(0)+m_MedianValues.at(1))/2;

		// if more than 2 elements, we need to sort them to find the middle.
		SortVals();

		T ret;
		if(number%2)
			ret = m_MedianValues.at(number/2);
		else
			ret = (m_MedianValues.at(number/2-1) + m_MedianValues.at(number/2))/2;

		return ret;
	}

	std::vector<T>	GetMedianVector() const
	{
		return m_MedianValues;
	}

	void	SortVals()
	{
		sort(m_MedianValues.begin(), m_MedianValues.end());
	}

private:
	std::vector<T>	m_MedianValues;
};